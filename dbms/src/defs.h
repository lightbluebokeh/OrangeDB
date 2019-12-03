#pragma once

#include <string>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <type_traits>

#ifndef NDEBUG
#define DEBUG
#endif

static_assert(sizeof(std::size_t) == 8, "x64 only");

#ifdef _MSC_VER
#pragma warning(disable : 26812)  // enum class
#endif

namespace fs = std::filesystem;

using rid_t = uint64_t;
using String = std::string;

using byte_t = uint8_t;
using bytes_t = byte_t*;
using const_bytes_t = const byte_t*;
using byte_arr_t = std::vector<byte_t>;
using rec_t = std::vector<byte_arr_t>;

struct page_t {
    int file_id, page_id;
};

struct buf_t {
    bytes_t bytes = nullptr;
    int buf_id;
};

const int MAX_DB_NUM = 5;
const int MAX_TBL_NUM = 12;
const int MAX_COL_NUM = 20;
// 最多同时打开的文件数目
const int MAX_FILE_NUM = MAX_TBL_NUM * (2 * MAX_COL_NUM + 3);

#include <iostream>

constexpr const char* RESET = "\033[0m";
constexpr const char* RED = "\033[31m";   /* Red */
constexpr const char* GREEN = "\033[32m"; /* Green */
constexpr const char* YELLOW = "\033[33m"; /* Yellow */
constexpr const char* CYAN = "\033[36m"; /* Cyan */

void ensure(bool cond, const String& msg);
#define DATA_INVALID 0xff

class OrangeException : public std::exception {
private:
    String msg;

public:
    OrangeException(const String& msg) : msg(msg) {}
    const char* what() const noexcept override { return msg.c_str(); }
};

class OrangeError : public std::exception {
private:
    String msg;
public:
    OrangeError(const String& msg) : msg(msg) {}
    const char* what() const noexcept override { return msg.c_str(); }
};
// for debug
inline void orange_assert(bool cond, const String& msg) {
#ifdef DEBUG
    if (!cond) throw OrangeError(msg);
#endif
}

#ifdef __GNUC__
#include <unistd.h>
#elif _MSC_VER
#include <io.h>
#endif

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

template <typename>
struct is_std_vector : std::false_type {};
template <typename T>
struct is_std_vector<std::vector<T>> : std::true_type {};
template <typename T>
constexpr bool is_std_vector_v = is_std_vector<std::remove_cv_t<std::remove_reference_t<T>>>::value;

template <typename>
struct is_basic_string : std::false_type {};
template <typename T>
struct is_basic_string<std::basic_string<T>> : std::true_type {};
template <typename T>
constexpr bool is_basic_string_v =
    is_basic_string<std::remove_cv_t<std::remove_reference_t<T>>>::value;

template <typename>
struct is_pair : std::false_type {};
template <typename T, typename U>
struct is_pair<std::pair<T, U>> : std::true_type {};
template <typename T>
constexpr bool is_pair_v = is_pair<std::remove_cv_t<std::remove_reference_t<T>>>::value;

constexpr int MAX_CHAR_LEN = 255;
constexpr int MAX_VARCHAR_LEN = 65535;

#define DATA_NULL 0x0
#define DATA_NORMAL 0x1

#define ORANGE_UNIMPL throw OrangeError("lazy orange coder has not implemented this yet");
#define ORANGE_UNREACHABLE throw OrangeError("ni za guo lai de");

template <class Fn, class... Args>
int expand(Fn&& func, Args&&... args) {
    int arr[]{(func(std::forward<Args&&>(args)), 0)...};
    return sizeof(arr) / sizeof(int);
}

enum datatype_t {
    ORANGE_INT,
    ORANGE_CHAR,
    ORANGE_VARCHAR,
    ORANGE_NUMERIC,
    ORANGE_DATE,
};

using numeric_t = long double;

namespace Orange {
    template <typename T>
    auto to_bytes(const T& t) {
        byte_arr_t ret(sizeof(T) + 1);
        ret[0] = DATA_NORMAL;
        memcpy(ret.data() + 1, (bytes_t)&t, sizeof(T));
        return ret;
    }

    template <>
    inline auto to_bytes(const String& str) {
        byte_arr_t ret(str.size() + 1);
        ret[0] = DATA_NORMAL;
        memcpy(ret.data() + 1, str.data(), str.size());
        return ret;
    }

    inline int bytes_to_int(const byte_arr_t& bytes) {
        orange_assert(bytes.size() == 5 && bytes.front() != DATA_NULL, "bad byte array for int");
        return *(int*)(bytes.data() + 1);
    }

    inline String bytes_to_string(const byte_arr_t& bytes) {
        orange_assert(bytes.front() != DATA_NULL, "null byte array for string");
        auto ret = String(bytes.begin() + 1, bytes.end());
        
        return ret;
    }
}

