#pragma once

#include <string>
#include <unordered_map>

#include <defs.h>

class RawRecord {
    bytes_t data;
    int* offset;

public:
    RawRecord(bytes_t data, int* offset) : data(data), offset(offset) {}

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
