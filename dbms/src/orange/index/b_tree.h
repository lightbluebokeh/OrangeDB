#pragma once

#include <fstream>

#include <utils/id_pool.h>

class BTree {
public:
    enum key_kind_t {
        BYTES,  // 可以对数据直接按照字节比较的类型
        FLOAT,  // 浮点类型，比较可能要再看看
        VARCHAR,   // varchar 地址类型
    };
private:
    String prefix;
    key_kind_t key_kind;
    size_t key_size;
    File *f_tree;

    using bid_t = long long;
    IdPool<bid_t> pool;
    const int t;

    String tree_name() { return prefix + ".bt"; }
    String pool_name() { return prefix + ".pl"; }
    String varchar_name() { return prefix + "vch"; }

    struct node_t {
        byte_t data[PAGE_SIZE];
        bid_t id;
        size_t key_size;

        node_t(bid_t id, size_t key_size) : id(id),  key_size(key_size) {}

        int& key_num() { return *(int*)data; }
        bid_t& ch(int i) {
            return *((bid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + key_size + sizeof(rid_t)));
        }
        bytes_t key(int i) {
            return data + sizeof(std::remove_reference_t<decltype(key_num())>) 
                        + i * (sizeof(bid_t) + key_size + sizeof(rid_t))
                        + sizeof(bid_t);
        }
        void set_key(int i, bytes_t key) {
            auto b = this->key(i);
            memcpy(b, key, key_size);
        }
        rid_t& val(int i) {
            return *((rid_t*)data + sizeof(std::remove_reference_t<decltype(key_num())>)
                                    + i * (sizeof(bid_t) + key_size + sizeof(rid_t))
                                    + sizeof(bid_t) + key_size);
        }
    };

    using node_ptr_t = std::unique_ptr<node_t>;
    

    node_ptr_t new_node() {
        auto id = pool.new_id();
        return std::make_unique<node_t>(id, key_size);
    }
    node_ptr_t read_node(bid_t id) {
        auto node = std::make_unique<node_t>(id, key_size);
        f_tree->seek_pos(id * PAGE_SIZE)->read_bytes(node->data, PAGE_SIZE);
        return node;
    }
    void write_node(node_ptr_t node) {
        f_tree->seek_pos(node->id * PAGE_SIZE)->write_bytes(node->data, PAGE_SIZE);
    }

    node_ptr_t root;
    void read_root() {
        std::ifstream is(prefix + ".meta");
        bid_t id;
        is >> id;
        root = read_node(id);
    }
    void write_root() {
        std::ofstream os(prefix + ".meta");
        os << root->id;
        write_node(std::move(root));
    }

    void insert_internal(node_ptr_t node, const byte_arr_t k, rid_t v) {
        UNIMPLEMENTED
    }

    int fanout(int key_size) {
        return (PAGE_SIZE - sizeof(std::remove_reference_t<decltype(std::declval<node_t>().key_num())>))
                / (2 * (sizeof(bid_t) + key_size + sizeof(rid_t)));
    }
public:
    BTree(const String& prefix, key_kind_t key_kind, size_t key_size) : 
        prefix(prefix),
        key_kind(key_kind), 
        key_size(key_size),
        pool(pool_name()),
        t(fanout(key_size)) {}

    ~BTree() {
        write_root();
        f_tree->close();
    }

    void init() {
        f_tree = File::create_open(tree_name());
        root = new_node();
        pool.init();
    }
    void load() {
        read_root();
        pool.load();
    }

    void insert(byte_arr_t k, rid_t v) {
        UNIMPLEMENTED
    }

    void remove(byte_arr_t k, rid_t v) {
        UNIMPLEMENTED
    }

    void update(byte_arr_t k, rid_t v) {
        UNIMPLEMENTED
    }

    std::vector<rid_t> query(byte_arr_t lo, byte_arr_t hi, cnt_t lim) {
        UNIMPLEMENTED
    }
};