#pragma once

#include <fstream>

#include "utils/id_pool.h"
#include "orange/table/column.h"
#include "orange/parser/sql_ast.h"

class Index;

class BTree {
private:
    Index &index;
    String path;
    int key_size;
    File *f_tree;

    using bid_t = long long;
    IdPool<bid_t> pool;
    const int t;

    String tree_name() { return path + "tree"; }
    String pool_name() { return path + "pool"; }
    // String varchar_name() { return path + "vch"; }

    int cmp(const std::vector<byte_arr_t>& ks1, rid_t v1, const std::vector<byte_arr_t>& ks2, rid_t v2) const;

    using pred_t = std::pair<ast::op, ast::data_value>;
    using preds_t = std::vector<pred_t>;

    struct node_t {
        BTree &tree;

        byte_t data[PAGE_SIZE];
        bid_t id;

        node_t(BTree &tree, bid_t id) : tree(tree), id(id) { memset(data, 0, sizeof(data)); }
        ~node_t() { tree.f_tree->seek_pos(id * PAGE_SIZE)->write_bytes(data, PAGE_SIZE); }

        int& key_num() { return *(int*)data; }
        const int& key_num() const { return *(int*)data; }
        bid_t& ch(int i) {
            return *(bid_t*)(data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t)));
        }
        const bid_t& ch(int i) const {
            return *(bid_t*)(data + sizeof(std::remove_reference_t<decltype(key_num())>)
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
            return *(rid_t*)(data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t))
                                    + sizeof(bid_t) + tree.key_size);
        }
        const rid_t& val(int i) const {
            return *(const rid_t*)(data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + tree.key_size + sizeof(rid_t))
                                    + sizeof(bid_t) + tree.key_size);
        }
        bool leaf() const { 
            return !ch(0) && !ch(1);    // 非叶子只可能有一个是零
        }
        bool full() const { return key_num() == tree.t * 2 - 1; }
        bool least() const { return key_num() == tree.t - 1; }
        void check_order();
    };

    using node_ptr_t = std::unique_ptr<node_t>;

    node_ptr_t new_node() {
        auto id = pool.new_id();
        return std::make_unique<node_t>(*this, id);
    }
    node_ptr_t read_node(bid_t id) const {
        auto node = std::make_unique<node_t>(*this, id);
        f_tree->seek_pos(id * PAGE_SIZE)->read_bytes(node->data, PAGE_SIZE);
        return node;
    }

    node_ptr_t root;
    void read_root() {
        std::ifstream ifs(path + "root");
        bid_t id;
        ifs >> id;
        root = read_node(id);
    }
    void write_root() {
        std::ofstream ofs(path + "root");
        ofs << root->id;
        root.release();
    }

    int fanout(int key_size) {
        return (PAGE_SIZE - sizeof(std::remove_reference_t<decltype(std::declval<node_t>().key_num())>))
                / (2 * (sizeof(bid_t) + key_size + sizeof(rid_t)));
    }

    // 使用**二分查找**找到最大的 i 使得 (k, v) > (key(j), val(j)) (i > j)
    int upper_bound(node_ptr_t &x, const std::vector<byte_arr_t>& ks, rid_t v);

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
        for (int j = x->key_num() - 1; j >= i; j--) {
            x->set_key(j + 1, x->key(j));
            x->val(j + 1) = x->val(j);
        }
        x->set_key(i, y->key(t - 1));
        x->val(i) = y->val(t - 1);
        x->key_num()++;
    }

    void merge(node_ptr_t& x, node_ptr_t& y, int i, node_ptr_t z) {
        y->set_key(t - 1, x->key(i));
        y->val(t - 1) = x->val(i);
        for (int j = 0; j < t - 1; j++) {
            y->ch(j + t) = z->ch(j);
            y->set_key(j + t, z->key(j));
            y->val(j + t) = z->val(j);
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

    std::pair<byte_arr_t, rid_t> min_raw(const node_ptr_t& x) {
        if (!x->leaf()) return min_raw(read_node(x->ch(0)));
        auto key = x->key(0);
        return std::make_pair(byte_arr_t(key, key + key_size), x->val(0));
    }

    std::pair<byte_arr_t, rid_t> max_raw(const node_ptr_t& x) {
        if (!x->leaf()) return max_raw(read_node(x->ch(x->key_num())));
        auto key = x->key(x->key_num() - 1);
        return std::make_pair(byte_arr_t(key, key + key_size), x->val(x->key_num() - 1));
    }

    void insert_nonfull(node_ptr_t &x, const_bytes_t k_raw, const std::vector<byte_arr_t>& ks, rid_t v);
    void remove_nonleast(node_ptr_t& x, const std::vector<byte_arr_t>& ks, rid_t v);
    // 内部查询，los 和 his 表示第一列的上下界
    void query(const node_ptr_t& x, const std::vector<preds_t>& preds_list, std::vector<rid_t>& result, rid_t lim) const;
    void check_order(node_ptr_t& x);
public:
    BTree(Index &index, int key_size, const String& path) : index(index), path(path),
        key_size(key_size), pool(pool_name()), t(fanout(key_size)) { orange_ensure(t >= 2, "btree fanout is too small"); }
    ~BTree() {
        write_root();
        f_tree->close();
    }

    void init() {
        f_tree = File::create_open(tree_name());
        pool.init();
        root = new_node();
    }
    void load() {
        f_tree = File::open(tree_name());
        read_root();
        pool.load();
    }

    void insert(const_bytes_t k_raw, rid_t v, const std::vector<byte_arr_t>& ks);
    void remove(const_bytes_t k_raw, rid_t v);

    std::vector<rid_t> query(const std::vector<preds_t>& preds_list, rid_t lim = MAX_RID) const;

    // auto query(Orange::parser::op op, const Orange::parser::data_value& value, rid_t lim) {
    //     using op_t = Orange::parser::op;
    //     std::vector<rid_t> ret;
    //     if (op == op_t::Neq) {
    //         query_internal(root, op_t::Lt, value, ret, lim);
    //         query_internal(root, op_t::Gt, value, ret, lim);
    //     } else {
    //         query_internal(root, op, value, ret, lim);
    //     }
    //     orange_assert(ret.size() <= lim, "query limit exeeded");
    //     return ret;
    // }
};