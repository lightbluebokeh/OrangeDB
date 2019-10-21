#pragma once

#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __linux__
#include <unistd.h>
#elif _WIN32
#include <io.h>
#endif

#include <defs.h>
/*
 * 一个页面中的字节数
 */
constexpr int PAGE_SIZE = 8192;

/*
 * 页面字节数以2为底的指数
 */
constexpr int PAGE_SIZE_IDX = 13;
static_assert((1 << PAGE_SIZE_IDX) == PAGE_SIZE);
constexpr int MAX_FMT_INT_NUM = 128;
// constexpr int BUF_PAGE_NUM = 65536;
// constexpr int MAX_FILE_NUM = 128;
constexpr int MAX_TYPE_NUM = 256;
/*
 * 缓存中页面个数上限
 */
constexpr int BUF_CAP = 60000;
constexpr int IN_DEBUG = 0;
constexpr int DEBUG_DELETE = 0;
constexpr int DEBUG_ERASE = 1;
constexpr int DEBUG_NEXT = 1;
/*
 * 一个表中列的上限
 */
constexpr int MAX_COL_NUM = 32;
constexpr int COL_SIZE = 128;
/*
 * 数据库中表的个数上限
 */
constexpr int MAX_TB_NUM = 32;
constexpr int RELEASE = 1;

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;
using int64 = long long;
using uint64 = unsigned long long;
