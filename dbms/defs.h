#pragma once

#include <string>

using rid_t = uint64_t;
using String = std::string;
using byte_t = unsigned char;
static_assert(sizeof(byte_t) == 1u);
using buf_t = byte_t*;
