#pragma once

#include <string>

using rid_t = uint64_t;
using String = std::string;

template<typename T>
constexpr bool is_byte_v = (sizeof(T) == 1);

using byte_t = uint8_t;
static_assert(is_byte_v<byte_t>);
using bytes_t = byte_t*;

using ByteArr = std::basic_string<byte_t>;
static_assert(std::is_same_v<ByteArr::value_type, byte_t>);

typedef struct {
    int file_id, page_id;
} page_t;
bool operator == (page_t a, page_t b) { return a.file_id == b.file_id && a.page_id == b.page_id; }
bool operator != (page_t a, page_t b) { return a.file_id != b.file_id || a.page_id != b.page_id; }

namespace std {
    template <>
    struct hash<page_t> {
        size_t operator() (const page_t& x) const { return x.file_id ^ x.page_id; }
    };

    // template<>
    // struct equal_to<page_t> {
    //     bool operator() (const page_t& a, const page_t& b) {
    //         return a.file_id == b.file_id && a.page_id == b.page_id;
    //     }
    // };

    // template<>
    // struct not_equal_to<page_t> {
    //     bool operator() (const page_t& a, const page_t& b) {
    //         return a.file_id != b.file_id || a.page_id != b.page_id;
    //     }
    // };
}  // namespace std


typedef struct {
    bytes_t bytes = nullptr;
    int buf_id;
} buf_t;

constexpr int MAX_FILE_NUM = 128;

#include <iostream>

#define RESET "\033[0m"
#define RED "\033[31m" /* Red */
#define GREEN "\033[32m" /* Green */

#define ensure(expr, msg) \
if ((expr) == 0) {        \
    std::cerr << RED << "failed: " << RESET << msg << std::endl;   \
    exit(1);              \
}
