#pragma once

#include <set>

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
    std::set<id_type> pool;

public:
    IdPool(String filename) : filename(filename) {}
    ~IdPool() { 
        auto offset = f_pool->write(0, tot, pool.size());
        std::vector<id_type> tmp(pool.begin(), pool.end());
        f_pool->write_bytes(offset, (bytes_t)tmp.data(), tmp.size() * sizeof(id_type));
        f_pool->close();
    }

    void init() {
        File::create(filename);
        f_pool = File::open(filename);
        tot = 0;
    }
    void load() {
        f_pool = File::open(filename);
        size_t pool_size;
        auto offset = f_pool->read(0, tot, pool_size);
        std::vector<id_type> tmp;
        tmp.resize(pool_size);
        f_pool->read_bytes(offset, (bytes_t)tmp.data(), pool_size * sizeof(id_type));
        pool = std::set<id_type>(tmp.begin(), tmp.end());
    }

    id_type new_id() {
        if (pool.empty()) return tot++;
        auto ret = *pool.begin();
        pool.erase(pool.begin());
        return ret;
    }

    void free_id(id_type id) {
        if (id == tot - 1) tot--;
        else pool.insert(id);
    }

    bool contains(id_type id) const { return id >= tot || pool.count(id); }

    // 返回所有正在使用的编号
    std::vector<id_type> all() const {
        // others 为不在返回值里的集合
        std::vector<id_type> ret;
        // others.resize(top);
        // f_pool->seek_pos(2 * sizeof(id_type))->read_bytes((bytes_t)others.data(), top * sizeof(id_type));
        auto others = std::vector<id_type>(pool.begin(), pool.end());
        for (id_type i = 0; i <= others.size(); i++) {
            for (id_type j = i == 0 ? 0 : others[i - 1] + 1, lim = i == others.size() ? tot : others[i]; j < lim; j++) {
                ret.push_back(j);
            }
        }
        return ret;
    }
};