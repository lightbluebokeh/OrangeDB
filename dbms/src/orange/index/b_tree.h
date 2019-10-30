#pragma once

#include <fs/file/file.h>

template<typename Key, typename Value>
class BTree {
public:
    using key_type = Key;
    using value_type = Value;
private:
    String name;
    File *file;

    int t;

    using bid_t = long long;
    bid_t root;

    void insert_internal(bit_t x, key_type k) {
        UNIMPLEMENTED
    }
public:

    BTree(const String& name) {
        UNIMPLEMENTED
    }

    void create() {
        UNIMPLEMENTED
    }

    void split(bid_t x, int i) {
        UNIMPLEMENTED
    }

    void insert(key_type k, value_type v) {
        UNIMPLEMENTED
    }

    void remove(key_type k, value_type v) {
        UNIMPLEMENTED
    }

    void update(key_type k, value_type v) {
        UNIMPLEMENTED
    }

    std::vector<value_type> query(value_type lo, value_type hi) {
        UNIMPLEMENTED
    }
};