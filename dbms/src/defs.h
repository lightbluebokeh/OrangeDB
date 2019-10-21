#pragma once

#include <string>

using rid_t = uint64_t;
using String = std::string;

template <typename T>
constexpr bool is_byte_v = (sizeof(T) == 1);

using byte_t = uint8_t;
static_assert(is_byte_v<byte_t>);
using bytes_t = byte_t*;

using ByteArr = std::basic_string<byte_t>;
static_assert(std::is_same_v<ByteArr::value_type, byte_t>);

typedef struct { int file_id, page_id; } page_t;


typedef struct { bytes_t bytes = nullptr; int buf_id; } buf_t;

constexpr int MAX_COL_NUM = 20;
constexpr int MAX_TB_NUM = 16;
constexpr int MAX_FILE_NUM = MAX_TB_NUM * (1 + 1 + 1 + MAX_COL_NUM); //  对于每张表：元数据+数据+编号+每个属性索引

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
