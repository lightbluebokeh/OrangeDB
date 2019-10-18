#pragma once

#include <defs.h>
#include <unordered_set>
#include <unordered_map>
#include <fs/utils/pagedef.h>

class BufPageMap {
private:
    // std::unordered_map<key_t, value_t> map;
    std::unordered_map<int, int> map[MAX_FILE_NUM];
    page_t map_inv[BUF_CAP];
    // std::unordered_set<int> pages[MAX_FILE_NUM];
public:
    int get_buf_id(page_t page) {
        // auto it = map.find(page);
        auto it = map[page.file_id].find(page.page_id);
        if (it == map[page.file_id].end()) return -1;
        return it->second;
    }

    page_t get_page(int buf_id) {
        return map_inv[buf_id];
    }

    void replace(int buf_id, page_t page) {
        map[page.file_id][page.page_id] = buf_id;
        map_inv[buf_id] = page;
        // map[page] = buf_id;
        // map_inv[buf_id] = page;
    }

    void remove_buf(int buf_id) {
        // map.erase(map_inv[buf_id]);
        // map[map_inv[buf_id].file_id].erase()
        auto page = map_inv[buf_id];
        map[page.file_id].erase(page.page_id);
        map_inv[buf_id] = {-1, -1};
    }

    BufPageMap() {
        for (int i = 0; i < BUF_CAP; ++i) {
            map_inv[i] = {-1, -1};
        }
    }
};

