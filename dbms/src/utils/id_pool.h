#pragma once

#include <fs/file/file.h>

template <typename T>
class IdPool {
public:
    using id_type = T;

private:
    String filename;
    File* f_pool;
    // 一般来说就那么多id，所以用同样的类型应该能存下
    // tot 是当前最大编号
    id_type top, tot;

public:
    IdPool(String filename) : filename(filename) {}
    ~IdPool() { f_pool->seek_pos(0)->write(top, tot)->close(); }

    void init() {
        File::create(filename);
        f_pool = File::open(filename);
        f_pool->seek_pos(0)->write(top = 0, tot = 0); 
    }
    void load() {
        f_pool = File::open(filename);
        f_pool->seek_pos(0)->read(top, tot); 
    }

    id_type new_id() {
        if (!top) return tot++;
        id_type ret = f_pool->seek_pos((top + 1) * sizeof(id_type))->template read<id_type>();
        top--;
        return ret;
    }

    void free_id(id_type id) {
        if (id == tot - 1)
            tot--;
        else {
            top++;
            f_pool->seek_pos((top + 1) * sizeof(id_type))->write(id);
        }
    }

    // 返回所有正在使用的编号
    std::vector<id_type> all() const {
        // others 为不在返回值里的集合
        std::vector<id_type> others, ret;
        others.resize(top);
        f_pool->seek_pos(2 * sizeof(id_type))->read_bytes((bytes_t)others.data(), top * sizeof(id_type));
        sort(others.begin(), others.end());
        for (id_type i = 0; i <= others.size(); i++) {
            for (id_type j = i == 0 ? 0 : others[i - 1] + 1, lim = i == others.size() ? tot : others[i]; j < lim; j++) {
                ret.push_back(j);
            }
        }
        return ret;
    }
};