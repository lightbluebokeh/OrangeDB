#pragma once

#include <string>
#include <type_traits>
#include <filesystem>

namespace fs = std::filesystem;

using rid_t = uint64_t;
using String = std::string;

template <typename T>
constexpr bool is_byte_v = (sizeof(T) == 1);

using byte_t = uint8_t;
static_assert(is_byte_v<byte_t>);
using bytes_t = byte_t*;
using byte_arr_t = std::vector<byte_t>;

typedef struct { int file_id, page_id; } page_t;

typedef struct { bytes_t bytes = nullptr; int buf_id; } buf_t;

constexpr int MAX_COL_NUM = 20;
constexpr int MAX_TBL_NUM = 12;
constexpr int MAX_DB_NUM = 5;
constexpr int MAX_FILE_NUM = MAX_DB_NUM * MAX_TBL_NUM * (4 + 2 * MAX_COL_NUM);

#include <iostream>

#define RESET "\033[0m"
#define RED "\033[31m"   /* Red */
#define GREEN "\033[32m" /* Green */

void ensure(bool cond, const String& msg);

#ifdef __linux__
#include <unistd.h>
#elif _WIN32
#include <io.h>
#endif

#include <defs.h>

constexpr int PAGE_SIZE = 8192;
constexpr int PAGE_SIZE_IDX = 13;
static_assert((1 << PAGE_SIZE_IDX) == PAGE_SIZE);

constexpr int BUF_CAP = 60000;
constexpr int IN_DEBUG = 0;
constexpr int DEBUG_DELETE = 0;
constexpr int DEBUG_ERASE = 1;
constexpr int DEBUG_NEXT = 1;

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;
using int64 = long long;
using uint64 = unsigned long long;
using uint8 = uint8_t;

// 没法重载赋值 /cy
constexpr int F_KEY_NAME_LIM = 32;
struct f_key_name_t { char data[F_KEY_NAME_LIM + 1]; };
constexpr int COL_NAME_LIM = 32;
struct col_name_t {
    char data[COL_NAME_LIM + 1]; 
    inline String get() const { return String(data); }
};
constexpr int TBL_NAME_LIM = 32;
struct tbl_name_t { 
    char data[TBL_NAME_LIM + 1]; 
    inline String get() { return "[" + String(data) + "]"; }
};
constexpr int COL_NAME_LIST_LIM = 5;
struct col_name_list_t {
    col_name_t data[COL_NAME_LIST_LIM]; 
    int size = 0;

    void add(col_name_t name) {
        if (size == COL_NAME_LIST_LIM) {
            throw "increase your constant";
        }
        data[size++] = name;
    }
};

template<typename>
struct is_std_vector : std::false_type {};
template<typename T>
struct is_std_vector<std::vector<T>> : std::true_type {};
template<typename T>
constexpr bool is_std_vector_v = is_std_vector<T>::value;

constexpr int MAX_CHAR_LEN = 256;