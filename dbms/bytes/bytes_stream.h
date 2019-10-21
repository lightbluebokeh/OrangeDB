#pragma once

#include <bytes/bytes_io.h>
#include <iostream>
#include <numeric>

// 偷懒 IO 共用一个 offset，请不要同时输入输出 /cy
class BytesStream {
private:
    bytes_t& bytes;
    size_t offset, lim;

    BytesStream(const BytesStream&) = delete;
    BytesStream& operator=(const BytesStream& bs) = delete;

    void check_overflow(size_t n) { ensure(n + offset <= lim, "seems bytes overflow re"); }

protected:
    virtual void before_IO(size_t n) { check_overflow(n); }
    virtual void after_O() {}

public:
    BytesStream(bytes_t& bytes, size_t lim) : bytes(bytes), lim(lim), offset(0) {}
    BytesStream(BytesStream&& bs) : bytes(bs.bytes), lim(bs.lim), offset(bs.offset) {}
    BytesStream& operator=(BytesStream&& bs) {
        bytes = bs.bytes;
        lim = bs.lim;
        offset = bs.offset;
        return *this;
    }
    virtual ~BytesStream() {}

    template <typename T>
    BytesStream& write_bytes(const std::vector<T>& vec) {
        return write_bytes(vec, vec.size());
    }
    template <typename T>
    BytesStream& write_bytes(const std::vector<T>& vec, size_t n) {
        before_IO(n);
        offset += BytesIO::write_bytes(bytes + offset, vec, n);
        after_O();
        return *this;
    }

    template <typename T>
    BytesStream& write_bytes(const std::basic_string<T>& str) {
        return write_bytes(str, str.size());
    }

    template <typename T>
    BytesStream& write_bytes(const std::basic_string<T>& str, size_t n) {
        before_IO(n);
        offset += BytesIO::write_bytes(bytes + offset, str, n);
        after_O();
        return *this;
    }

    template <typename T>
    BytesStream& write(const T& t, size_t n = sizeof(T)) {
        before_IO(n);
        offset += BytesIO::write(bytes + offset, t, n);
        after_O();
        return *this;
    }

    BytesStream& memset(byte_t c = 0) { return memset(c, lim - offset); }

    BytesStream& memset(byte_t c, size_t n) {
        before_IO(n);
        offset += BytesIO::memset(bytes + offset, c, n);
        after_O();
        return *this;
    }

    template <typename T>
    T read() {
        before_IO(sizeof(T));
        T ret = BytesIO::read<T>(bytes + offset);
        offset += sizeof(T);
        return ret;
    }

    // 读取 sizeof(T) 个字节传给 U
    template <typename T, typename U = T>
    BytesStream& read(U& u) {
        before_IO(sizeof(T));
        offset += BytesIO::read<T, U>(bytes + offset, u);
        return *this;
    }

    template <typename T>
    BytesStream& read_bytes(std::vector<T>& vec, size_t n) {
        before_IO(n);
        offset += BytesIO::read_bytes(bytes + offset, vec, n);
        return *this;
    }

    template <typename T>
    BytesStream& read_bytes(std::basic_string<T>& str, size_t n) {
        before_IO(n);
        offset += BytesIO::read_bytes(bytes + offset, str, n);
        return *this;
    }

    BytesStream& seekpos(size_t pos = 0) {
        ensure(pos <= lim, "seek overflow re");
        offset = pos;
        return *this;
    }

    BytesStream& seekoff(int off) {
        ensure(off >= 0 && off <= lim - offset || off < 0 && -off <= offset, "seek overflow re");
        offset += off;
        return *this;
    }
};