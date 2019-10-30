#pragma once

#include <fs/file/file.h>

template<typename Key, typename Value>
class BTree {
public:
    using key_t = Key;
    using value_t = Value;
private:
    String name;
    File *file;
    using bid_t = long long;

    // 最小分叉数
    constexpr static int t = (PAGE_SIZE - sizeof(int)) / (2 * (sizeof(key_t) + sizeof(value_t) + sizeof(bid_t)));

    struct node_t {
        byte_t data[PAGE_SIZE];

        int& key_num() {
            return *(int*)data;
        }
    };

    node_t* read(bid_t id) {
        
    }

    std::pair<bid_t, node_t*> new_node() {

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

    // void split(bid_t x, int i) {
    //     UNIMPLEMENTED
    // }

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