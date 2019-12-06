#pragma once

#include "fs/file/file.h"

template <typename T>
class IdPool {
public:
    using id_type = T;

private:
    String filename;
    File* f_pool;
    // 一般来说就那么多id，所以用同样的类型应该能存下
    // tot 是当前最大编号
    // id_type top, tot;
    id_type tot;
    std::vector<id_type> pool;

public:
    IdPool(String filename) : filename(filename) {}
    ~IdPool() { 
        // f_pool->write(0, top, tot)->close(); 
        f_pool->write(0, tot, pool);
    }

    void init() {
        File::create(filename);
        f_pool = File::open(filename);
        // f_pool->write(0, top = 0, tot = 0); 
    }
    void load() {
        f_pool = File::open(filename);
        f_pool->read(0, tot, pool);
    }

    id_type new_id() {
        if (pool.empty()) return tot++;
        auto ret = pool.back();
        pool.pop_back();
        return ret;
    }

    void free_id(id_type id) {
        if (id == tot - 1) tot--;
        else pool.push_back(id);
    }

    // 返回所有正在使用的编号
    std::vector<id_type> all() const {
        // others 为不在返回值里的集合
        std::vector<id_type> ret;
        // others.resize(top);
        // f_pool->seek_pos(2 * sizeof(id_type))->read_bytes((bytes_t)others.data(), top * sizeof(id_type));
        auto others = pool;
        std::sort(others.begin(), others.end());
        for (id_type i = 0; i <= others.size(); i++) {
            for (id_type j = i == 0 ? 0 : others[i - 1] + 1, lim = i == others.size() ? tot : others[i]; j < lim; j++) {
                ret.push_back(j);
            }
        }
        return ret;
    }
};