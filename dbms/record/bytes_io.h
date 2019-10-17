#pragma once

#include <defs.h>
#include <string>
#include <vector>
#include <cstring>
#include <type_traits>

namespace BytesIO {
    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(bytes_t bytes, const std::basic_string<T>& str, size_t n) {
        assert(str.size() <= n);
        memcpy(bytes, str.data(), str.size());
        memset(bytes + str.size(), 0, n - str.size());
        // bytes += n;
        return n;
    }

    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(bytes_t bytes, const std::vector<T>& vec, size_t n) {
        assert(vec.size() <= n);
        memcpy(bytes, vec.data(), vec.size());
        memset(bytes + vec.size(), 0, n - vec.size());
        // bytes += n;
        return n;
    }

    template<typename T>
    size_t write_obj(bytes_t bytes, const T& t, size_t n = sizeof(T)) {
        assert(sizeof(T) <= n);
        memcpy(bytes, &t, n);
        // bytes += n;
        return n;
    }

    // size_t move(bytes_t bytes, size_t n) {
    //     memset(bytes, 0, n);
    //     bytes += n;
    //     return n;
    // }
}
