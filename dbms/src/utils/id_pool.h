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
};