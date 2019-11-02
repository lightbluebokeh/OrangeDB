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
        const int& key_num() const { return *(int*)data; }
        bid_t& ch(int i) {
            return *((bid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t)));
        }
        const bid_t& ch(int i) const {
            return *((bid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t)));
        }
        bytes_t key(int i) {
            return data + sizeof(std::remove_reference_t<decltype(key_num())>) 
                        + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t))
                        + sizeof(bid_t);
        }
        const_bytes_t key(int i) const {
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
        const rid_t& val(int i) const {
            return *((rid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t))
                                    + sizeof(bid_t) + tree.key_size);
        }
        bool leaf() const { return !ch(0) && !ch(1); }
        bool full() const { return key_num() == tree.t * 2 - 1; }
        bool least() const { return key_num() == tree.t - 1; }
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
        for (int j = 0; j < t - 1; j++) {
            z->set_key(j, y->key(j + t));
            z->val(j) = y->val(j + t);
        }
        z->key_num() = t - 1;
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
    int upper_bound(node_ptr_t &x, const byte_arr_t& k, rid_t v) {
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
        int i = upper_bound(x, k, v);
        ensure(!i || cmp(k, v, x->key(i - 1), x->val(i - 1)) != 0, "try to insert something already exists");
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

    std::pair<byte_arr_t, rid_t> min(const node_ptr_t& x) {
        if (!x->leaf()) return min(read_node(x->ch(0)));
        auto key = x->key(0);
        return std::make_pair(byte_arr_t(key, key + key_size), x->val(0));
    }

    std::pair<byte_arr_t, rid_t> max(const node_ptr_t& x) {
        if (!x->leaf()) return min(read_node(x->ch(x->key_num())));
        auto key = x->key(x->key_num() - 1);
        return std::make_pair(byte_arr_t(key, key + key_size), x->val(x->key_num() - 1));
    }

    void merge(node_ptr_t& x, node_ptr_t& y, int i, node_ptr_t z) {
        y->set_key(t, x->key(i));
        y->val(t) = x->val(i);
        for (int j = 0; j < t - 1; j++) {
            y->ch(j + t) = z->ch(j);
            y->set_key(j + t + 1, z->key(j));
            y->val(j + t + 1) = z->val(j);
        }
        y->ch(2 * t - 1) = z->ch(t - 1);
        y->key_num() = 2 * t - 1;
        pool.free_id(z->id);
        for (int j = i; j + 1 < x->key_num(); j++) {
            x->set_key(j, x->key(j + 1));
            x->val(j) = x->val(j + 1);
            x->ch(j + 1) = x->ch(j + 2);
        }
        x->key_num()--;
    }

    void remove_nonleast(node_ptr_t& x, const byte_arr_t& k, rid_t v) {
        int i = upper_bound(x, k, v);
        if (i && cmp(k, v, x->key(i - 1), x->val(i - 1)) == 0) {
            if (x->leaf()) {
                for (int j = i; j + 1 < x->key_num(); j++) {
                    x->set_key(j, x->key(j + 1));
                    x->val(j) = x->val(j + 1);
                }
                x->key_num()--;
            } else {
                auto y = read_node(x->ch(i));
                if (!y->least()) {
                    auto [k_raw, v] = min(y);
                    x->set_key(i, k_raw.data());
                    x->val(i) = v;
                    remove_nonleast(y, convert(k_raw.data()), v);
                } else {
                    auto z = read_node(x->ch(i + 1));
                    if (!z->least()) {
                        auto [k_raw, v] = max(z);
                        x->set_key(i, k_raw.data());
                        x->val(i) = v;
                        remove_nonleast(z, convert(k_raw.data()), v);
                    } else {
                        merge(x, y, i, std::move(z));
                        remove_nonleast(y, k, v);
                        if (x->key_num() == 0) std::swap(x, y);
                    }
                }
            } 
        } else {
            ensure(!x->leaf(), "trying to delete something does not exist");
            auto y = read_node(x->ch(i));
            if (!y->least()) remove_nonleast(y, k, v);
            else {
                node_ptr_t l, r;
                auto from_l = [&] {
                    if (i == 0) return 0;
                    l = read_node(x->ch(i - 1));
                    if (l->least()) return 0;
                    y->ch(t) = y->ch(t - 1);
                    for (int j = t - 1; j > 0; j--) {
                        y->set_key(j, y->key(j - 1));
                        y->val(j) = y->val(j - 1);
                        y->ch(j) = y->ch(j - 1);
                    }
                    y->set_key(0, x->key(i - 1));
                    y->val(0) = x->val(i - 1);
                    y->ch(0) = l->ch(l->key_num());
                    y->key_num()++;
                    x->set_key(i - 1, l->key(l->key_num() - 1));
                    x->val(i - 1) = l->val(l->key_num() - 1);
                    l->key_num()--;
                    return 1;
                };
                auto from_r = [&] {
                    if (i == 0) return 0;
                    r = read_node(x->ch(i + 1));
                    if (r->least()) return 0;
                    y->set_key(y->key_num(), x->key(i));
                    y->val(y->key_num()) = x->val(i);
                    y->ch(y->key_num() + 1) = r->ch(0);
                    y->key_num()++;
                    x->set_key(i, r->key(0));
                    x->val(i) = r->val(0);
                    for (int j = 0; j + 1 < r->key_num(); j++) {
                        r->ch(j) = r->ch(j + 1);
                        r->set_key(j, r->key(j + 1));
                        r->val(j) = r->val(j + 1);
                    }
                    r->ch(r->key_num() - 1) = r->ch(r->key_num());
                    r->key_num()--;
                    return 1;
                };

                if (from_l() || from_r()) remove_nonleast(y, k, v);
                else if (i != 0) {
                    merge(x, l, i - 1, std::move(y));
                    remove_nonleast(l, k, v);
                    if (x->key_num() == 0) std::swap(x, l);
                } else {
                    merge(x, y, i, std::move(r));
                    remove_nonleast(y, k, v);
                    if (x->key_num() == 0) std::swap(x, y);
                }
            }
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
            std::swap(s, root);
            root->ch(0) = s->id;
            split(root, s, 0);
        }
        insert_nonfull(root, k_raw, convert(k_raw), v);
    }

    void remove(const_bytes_t k_raw, rid_t v) {
        remove_nonleast(root, convert(k_raw), v);
    }

    std::vector<rid_t> query(const byte_arr_t& lo, const byte_arr_t& hi, rid_t lim) {
        UNIMPLEMENTED
    }
};