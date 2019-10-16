#pragma once

#include <defs.h>
#include <string>
#include <cstring>
#include <type_traits>

using Bytes = std::basic_string<byte_t>;
template<class T>
constexpr bool is_bytable_v = std::disjunction_v<std::is_integral<T>, 
                                                std::is_floating_point<T>, 
                                                std::is_same<T, Bytes>>;

template<typename T>
void write_buf(buf_t& buf, const std::enable_if_t<is_bytable_v<T>, T>& t, size_t n) {
    assert(sizeof(T) <= n);
    memcpy(buf, &t, sizeof(T));
    memset(buf + sizeof(T), 0, n - sizeof(T));
    buf += n;
}

template<>
void write_buf<Bytes>(buf_t& buf, const Bytes& bytes, size_t n) {
    assert(bytes.size() <= n);
    memcpy(buf, bytes.data(), n);
    memset(buf + bytes.size(), 0, n - bytes.size());
    buf += n;
}

template<typename T>
void write_buf(buf_t& buf, const std::enable_if_t<is_bytable_v<T>, T>& t) {
    write_buf(buf, t, sizeof(T));
}
