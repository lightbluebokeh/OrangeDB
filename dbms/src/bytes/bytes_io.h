#pragma once

#include <cstring>
#include <defs.h>
#include <string>
#include <type_traits>
#include <vector>

namespace BytesIO {
    template <typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(bytes_t bytes,
                                                       const std::basic_string<T>& str, size_t n) {
        if (str.size() <= n) {
            memcpy(bytes, str.data(), str.size());
            memset(bytes + str.size(), 0, n - str.size());
        } else {
            memcpy(bytes, str.data(), n);
        }
        return n;
    }

    template <typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(bytes_t bytes, const std::vector<T>& vec,
                                                       size_t n) {
        if (vec.size() <= n) {
            memcpy(bytes, vec.data(), vec.size());
            memset(bytes + vec.size(), 0, n - vec.size());
        } else {
            memcpy(bytes, vec.data(), n);
        }
        return n;
    }

    template <typename T>
    size_t write(bytes_t bytes, const T& t, size_t n = sizeof(T)) {
        memcpy(bytes, &t, n);
        return n;
    }

    template <typename T>
    T read(bytes_t bytes) {
        return *(T*)bytes;
    }

    template <typename T, typename U = T>
    size_t read(bytes_t bytes, U& u) {
        u = read<T>(bytes);
        return sizeof(T);
    }

    template <typename T>
    std::enable_if_t<is_byte_v<T>, size_t> read_bytes(bytes_t bytes, std::vector<T>& vec,
                                                      size_t n) {
        vec = std::vector<T>(bytes, n);
        return n;
    }

    template <typename T>
    std::enable_if_t<is_byte_v<T>, size_t> read_bytes(bytes_t bytes, std::basic_string<T>& str,
                                                      size_t n) {
        str = std::basic_string<T>(bytes, n);
        return n;
    }

    size_t memset(bytes_t bytes, byte_t c, size_t n) {
        ::memset(bytes, c, n);
        return n;
    }
}  // namespace BytesIO
