#pragma once

#include <fstream>

#include <utils/id_pool.h>

class BTree {
private:
    String prefix;
    index_key_kind_t kind;
    size_t key_size;
    File *f_tree, *f_vch;

    using bid_t = long long;
    IdPool<bid_t> pool;
    const int t;

    String tree_name() { return prefix + ".bt"; }
    String pool_name() { return prefix + ".pl"; }
    String varchar_name() { return prefix + "vch"; }

    struct node_t {
        BTree &tree;

        byte_t data[PAGE_SIZE];
        bid_t id;

        node_t(BTree &tree, bid_t id) : tree(tree), id(id) { memset(data, 0, sizeof(data)); }
        ~node_t() { tree.f_tree->seek_pos(id * PAGE_SIZE)->write_bytes(data, PAGE_SIZE); }

        int& key_num() { return *(int*)data; }
        bid_t& ch(int i) {
            return *((bid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t)));
        }
        bytes_t key(int i) {
            return data + sizeof(std::remove_reference_t<decltype(key_num())>) 
                        + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t))
                        + sizeof(bid_t);
        }
        void set_key(int i, const_bytes_t key) { memcpy(this->key(i), key, tree.key_size); }
        rid_t& val(int i) {
            return *((rid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t))
                                    + sizeof(bid_t) + tree.key_size);
        }
        bool leaf() { return ch(1) == 0; }
        bool full() { return key_num() == tree.t * 2 - 1; }
    };

    using node_ptr_t = std::unique_ptr<node_t>;

    node_ptr_t new_node() {
        auto id = pool.new_id();
        return std::make_unique<node_t>(*this, id);
    }
    node_ptr_t read_node(bid_t id) {
        auto node = std::make_unique<node_t>(*this, id);
        f_tree->seek_pos(id * PAGE_SIZE)->read_bytes(node->data, PAGE_SIZE);
        return node;
    }

    node_ptr_t root;
    void read_root() {
        std::ifstream is(prefix + ".root");
        bid_t id;
        is >> id;
        root = read_node(id);
    }
    void write_root() {
        std::ofstream os(prefix + ".root");
        os << root->id;
    }

    int fanout(int key_size) {
        return (PAGE_SIZE - sizeof(std::remove_reference_t<decltype(std::declval<node_t>().key_num())>))
                / (2 * (sizeof(bid_t) + key_size + sizeof(rid_t)));
    }

    // y 是 x 的 i 号儿子
    void split(node_ptr_t& x, node_ptr_t& y, int i) {
        node_ptr_t z = new_node();
        z->key_num() = t - 1;
        for (int j = 0; j < t - 1; j++) {
             z->set_key(j, y->key(j + t));
             z->val(j) = y->val(j + t);
        }
        if (!y->leaf()) {
            for (int j = 0; j < t; j++) z->ch(j) = y->ch(j + t);
        }
        y->key_num() = t - 1;
        for (int j = x->key_num(); j > i; j--) {
            x->ch(j + 1) = x->ch(j);
        }
        x->ch(i + 1) = z->id;
        for (int j = x->key_num() - 1; j >= i; i--) {
            x->set_key(j + 1, x->key(j));
            x->val(j + 1) = x->val(j);
        }
        x->set_key(i, y->key(t - 1));
        x->val(i) = y->val(t - 1);
        x->key_num()++;
    }

    // 使用**二分查找**找到最大的 i 使得 (k, v) > (key(j), val(j)) (i > j)
    int lower_bound(node_ptr_t &x, const byte_arr_t& k, rid_t v) {
        int i = 0;
        int l = 0, r = x->key_num() - 1;
        while (l <= r) {
            int m = (l + r) >> 1;
            if (cmp(k, v, x->key(m), x->val(m)) > 0) i = m, l = m + 1;
            else r = m - 1;
        }
        return i;
    }
    
    auto convert(const_bytes_t k_raw) {
        switch (kind) {
            case index_key_kind_t::BYTES: return byte_arr_t(k_raw, k_raw + key_size);
            case index_key_kind_t::NUMERIC: UNIMPLEMENTED
            case index_key_kind_t::VARCHAR: UNIMPLEMENTED
        }
    }

    int cmp_key(const byte_arr_t& k1, const_bytes_t k2_raw) {
        switch (kind) {
            case index_key_kind_t::BYTES: return bytesncmp(k1.data(), k2_raw, key_size);
            case index_key_kind_t::NUMERIC: UNIMPLEMENTED
            case index_key_kind_t::VARCHAR: UNIMPLEMENTED        
        }        
    }

    int cmp(const byte_arr_t& k1, rid_t v1, const_bytes_t k2_raw, rid_t v2) {
        int key_code = cmp_key(k1, k2_raw);
        if (key_code != 0) return key_code;
        return v1 - v2;
    }

    void insert_nonfull(node_ptr_t &x, const_bytes_t k_raw, const byte_arr_t& k, rid_t v) {
        int i = lower_bound(x, k, v);
        if (x->leaf()) {
            for (int j = x->key_num() - 1; j >= i; i--) {
                x->set_key(j + 1, x->key(j));
                x->val(j + 1) = x->val(j);
            }
            x->set_key(i, k_raw);
            x->val(i) = v;
            x->key_num()++;
        } else {
            auto y = read_node(x->ch(i));
            if (y->full()) {
                split(x, y, i);
                if (cmp(k, v, x->key(i), x->val(i)) > 0) {
                    i++;
                    y = read_node(x->ch(i));
                }
            }
            insert_nonfull(y, k_raw, k, v);
        }
    }
public:
    BTree(index_key_kind_t kind, size_t key_size, const String& prefix) : prefix(prefix), kind(kind), 
        key_size(key_size), pool(pool_name()), t(fanout(key_size)) { ensure(t >= 2, "fanout too few"); }
    ~BTree() {
        write_root();
        f_tree->close();
        if (kind == index_key_kind_t::VARCHAR) {
            f_vch->close();
        }
    }

    void init(File* f_data) {
        f_tree = File::create_open(tree_name());
        if (kind == index_key_kind_t::VARCHAR) f_vch = File::create_open(varchar_name());
        root = new_node();
        pool.init();
        bytes_t key = new byte_t[key_size];
        f_data->seek_pos(0);
        for (rid_t i = 0, tot = f_data->size() / key_size; i < tot; i++) {
            f_data->read_bytes(key, key_size);
            if (*key != DATA_INVALID) insert(key, i);
        }
        delete key;
    }
    void load() {
        f_tree = File::open(tree_name());
        if (kind == index_key_kind_t::VARCHAR) f_vch = File::open(varchar_name());
        read_root();
        pool.load();
    }

    void insert(const_bytes_t k_raw, rid_t v) {
        if (root->full()) {
            auto s = new_node();
            swap(s, root);
            root->ch(0) = s->id;
            split(root, s, 0);
        }
        insert_nonfull(root, k_raw, convert(k_raw), v);
    }

    void remove(const_bytes_t k_raw, rid_t v) {
        UNIMPLEMENTED
    }

    // 好像这个方法对于 B 树没有意义
    // void update(const byte_arr_t& k_raw, rid_t v) {
    //     UNIMPLEMENTED
    // }

    std::vector<rid_t> query(const byte_arr_t& lo, const byte_arr_t& hi, rid_t lim) {
        UNIMPLEMENTED
    }
};