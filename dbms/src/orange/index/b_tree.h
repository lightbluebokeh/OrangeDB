#pragma once

#include <fs/file/file.h>
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
    File *file;

    using bid_t = long long;
    IdPool<bid_t> pool;
    const int t;

    String pool_name() { return prefix + ".pl"; }

    struct node_t {
        byte_t data[PAGE_SIZE];
        bid_t id;
        size_t key_size;

        node_t(bid_t id, size_t key_size) : id(id),  key_size(key_size) {}

        // void read(File* file) { file->seek_pos(id * PAGE_SIZE)->read_bytes(data, PAGE_SIZE); }

        // void write(File* file) { file->seek_pos(id * PAGE_SIZE)->write_bytes(data, PAGE_SIZE); }

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
        file->seek_pos(id * PAGE_SIZE)->read_bytes(node->data, PAGE_SIZE);
        return node;
    }

    // borrowed
    void write_node(node_ptr_t node) {
        file->seek_pos(node->id * PAGE_SIZE)->write_bytes(node->data, PAGE_SIZE);
    }

    void insert_internal(node_ptr_t node, const byte_arr_t key, bid_t val) {
        UNIMPLEMENTED
    }
public:

    BTree(const String& prefix, key_kind_t key_kind) : 
        prefix(prefix), 
        key_kind(key_kind), 
        pool(pool_name()),
        t((PAGE_SIZE - sizeof(std::remove_reference_t<decltype(std::declval<node_t>().key_num())>)) / (2 * (sizeof(bid_t) + key_size + sizeof(rid_t)))) {}

    void create() {
        UNIMPLEMENTED
    }

    // void insert(key_t k, value_t v) {
    //     UNIMPLEMENTED
    // }

    // void remove(key_t k, value_t v) {
    //     UNIMPLEMENTED
    // }

    // void update(key_t k, value_t v) {
    //     UNIMPLEMENTED
    // }

    // std::vector<value_t> query(value_t lo, value_t hi) {
    //     UNIMPLEMENTED
    // }
};