#pragma once

#include <fs/file/file.h>

template<typename T>
class IdPool {
public:
    using id_type = T;
private:
    File *f_pool;
    // 一般来说就那么多id，所以用同样的类型应该能存下 
    id_type top, tot;

public:
    IdPool(String filename) { f_pool = File::open(filename); }
    ~IdPool() { f_pool->close(); }

    void init() { f_pool->seek_pos(0)->write(top = 0, tot = 0); }
    void load() { f_pool->seek_pos(0)->read(top, tot); }
    void save() { f_pool->seek_pos(0)->write(top, tot); }

    id_type new_id() {
        if (!top) return tot++;
        id_type ret = f_pool->seek_pos((top + 1) * sizeof(id_type))->template read<id_type>();
        top--;
        return ret;
    }
    void free_id(id_type id) {
        if (id == tot) tot--;
        else {
            top++;
            f_pool->seek_pos((top + 1) * sizeof(id_type))->write(id);
        }
    }
};