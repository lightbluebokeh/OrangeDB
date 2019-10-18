#pragma once

#include <string>
#include <unordered_map>

#include <defs.h>
#include <fs/utils/pagedef.h>

class RawRecord {
    rid_t id;
    bytes_t data;
    int* offset;

public:
    RawRecord(rid_t id, bytes_t data, int* offset) : id(id), data(data), offset(offset) {}

    rid_t getid() const { return id; }

    template <class T>
    T& get(int n) {
        return *((T*)(data + offset[n]));
    }

    template <class T>
    const T& get(int n) const {
        return *((T*)(data + offset[n]));
    }

    template <class T>
    T& get_offset(int offset) {
        return *((T*)(data + offset));
    }

    template <class T>
    const T& get_offset(int offset) const {
        return *((T*)(data + offset));
    }
};
