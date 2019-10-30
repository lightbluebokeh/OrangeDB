#pragma once

#include <boost/math/bindings/mpfr.hpp>
#include <boost/math/special_functions/gamma.hpp>

#include <fs/file/file.h>
#include <utils/id_pool.h>

template<typename K, typename V>
class BTree {
public:
    using key_t = K;
    using value_t = V;
private:
    String name;
    File *file;

    using bid_t = long long;
    IdPool<bid_t> pool;
    // 最小分叉数
    constexpr static int t = (PAGE_SIZE - sizeof(int)) / (2 * (sizeof(key_t) + sizeof(value_t) + sizeof(bid_t)));
    struct node_t {
        bid_t id;
        int key_num;
        bid_t ch[2 * t];
        key_t key[2 * t - 1];
        value_t val[2 * t - 1];
        static_assert(sizeof(key_num) + sizeof(ch) + sizeof(key) + sizeof(val) <= PAGE_SIZE);

        node_t(bid_t id) : id(id) {}

        void read(File* file) { file->seek_pos(id * PAGE_SIZE)->read(key_num, ch, key, val); }
        void write(File* file) { file->seek_pos(id * PAGE_SIZE)->write(key_num, ch, key, val); }
    };

    std::unique_ptr<node_t> new_node() {
        auto id = pool.new_id();
        return std::make_unique<node_t>(id);
    }

    std::unique_ptr<node_t> read_node(bid_t id) {
        // std::uni
        // node_t *node = new node_t(id);
        // node->read(file);
        // return node;
    }

    void insert_internal(bid_t x, key_t k, value_t v) {
        UNIMPLEMENTED
    }
public:

    BTree(const String& name) {
        UNIMPLEMENTED
    }

    void create() {
        UNIMPLEMENTED
    }

    void insert(key_t k, value_t v) {
        UNIMPLEMENTED
    }

    void remove(key_t k, value_t v) {
        UNIMPLEMENTED
    }

    void update(key_t k, value_t v) {
        UNIMPLEMENTED
    }

    std::vector<value_t> query(value_t lo, value_t hi) {
        UNIMPLEMENTED
    }
};