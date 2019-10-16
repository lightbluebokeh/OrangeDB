#pragma once

#include <defs.h>
#include <string>
#include <cstring>

using Bytes = std::basic_string<byte_t>;

void write_buf(buf_t buf, const Bytes& bytes, size_t n) {
    memcpy(buf, bytes.data(), n);
    memset(buf + bytes.size(), 0, n - bytes.size());
}

void write_buf(buf_t buf, const Bytes& bytes) {
    write_buf(buf, bytes, bytes.size());
}