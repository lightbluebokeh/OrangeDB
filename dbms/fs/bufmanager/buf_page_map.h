#pragma once

#include <defs.h>
#include <unordered_map>
#include <fs/utils/pagedef.h>



class BufPageMap {
public:
    using key_t = page_t;
    using value_t = int;
private:

    std::unordered_map<key_t, value_t> map;
    key_t map_inv[BUF_CAP];
public:
    value_t get_buf_id(key_t page) {
        auto it = map.find(page);
        if (it == map.end()) return -1;
        return it->second;
    }

    key_t get_page(value_t buf_id) {
        return map_inv[buf_id];
    }

    void replace(value_t buf_id, key_t page) {
        map[page] = buf_id;
        map_inv[buf_id] = page;
    }

    void remove_buf(value_t buf_id) {
        map.erase(map_inv[buf_id]);
        map_inv[buf_id] = {-1, -1};
    }

    BufPageMap() {
        for (int i = 0; i < BUF_CAP; ++i) {
            map_inv[i] = {-1, -1};
        }
    }
};

