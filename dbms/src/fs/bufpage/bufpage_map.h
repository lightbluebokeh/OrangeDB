#pragma once

#include <defs.h>
#include <unordered_map>
#include <unordered_set>

class BufpageMap {
private:
    std::unordered_map<int, int> map[MAX_FILE_NUM];
    page_t map_inv[BUF_CAP];

public:
    bool contains(page_t page) { return map[page.file_id].count(page.page_id); }

    // 找 page 的 buf id，找不到就返回 -1
    int get_buf_id(page_t page) {
        auto it = map[page.file_id].find(page.page_id);
        if (it == map[page.file_id].end()) return -1;
        return it->second;
    }

    page_t get_page(int buf_id) { return map_inv[buf_id]; }

    void set_map(page_t page, int buf_id) {
        auto pre = map_inv[buf_id];
        if (pre != page_t{-1, -1}) map[pre.file_id].erase(pre.page_id);
        map[page.file_id][page.page_id] = buf_id;
        map_inv[buf_id] = page;
    }

    BufpageMap() {
        for (int i = 0; i < BUF_CAP; ++i) {
            map_inv[i] = {-1, -1};
        }
    }
};
