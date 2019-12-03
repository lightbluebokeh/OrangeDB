#pragma once

#include <iostream>

// 偷懒 IO 共用一个 offset，请不要同时输入输出 /cy
class BytesStream {
private:
    bytes_t& bytes;
    size_t lim, offset;

    BytesStream(const BytesStream&) = delete;
    BytesStream& operator=(const BytesStream& bs) = delete;

    void check_overflow(size_t n) { orange_ensure(n + offset <= lim, "seems bytes overflow re"); }

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

    BytesStream& write_bytes(const std::remove_pointer_t<bytes_t>* bytes, size_t n) {
        before_IO(n);
        memcpy(this->bytes + offset, bytes, n);
        offset += n;
        after_O();
        return *this;
    }

    template <typename T>
    BytesStream& write(const T& t, size_t n = sizeof(T)) {
        before_IO(n);
        memcpy(bytes + offset, &t, n);
        offset += n;
        after_O();
        return *this;
    }

    BytesStream& memset(byte_t c = 0) { return memset(c, lim - offset); }

    BytesStream& memset(byte_t c, size_t n) {
        before_IO(n);
        ::memset(bytes + offset, c, n);
        offset += n;
        after_O();
        return *this;
    }

    template <typename T>
    T read() {
        before_IO(sizeof(T));
        T ret = *(T*)(bytes + offset);
        offset += sizeof(T);
        return ret;
    }

    // 读取 sizeof(T) 个字节传给 U
    template <typename T, typename U = T>
    BytesStream& read(U& u) {
        before_IO(sizeof(T));
        // offset += BytesIO::read<T, U>(bytes + offset, u);
        u = *(T*)(bytes + offset);
        offset += sizeof(T);
        return *this;
    }

    BytesStream& read_bytes(bytes_t bytes, size_t n) {
        before_IO(n);
        memcpy(bytes, this->bytes + offset, n);
        offset += n;
        return *this;
    }

    BytesStream& seekpos(size_t pos = 0) {
        orange_ensure(pos <= lim, "seek overflow re");
        offset = pos;
        return *this;
    }

    BytesStream& seekoff(int off) {
        orange_ensure((off >= 0 && size_t(off) <= lim - offset) || (off < 0 && size_t(-off) <= offset), "seek overflow re");
        offset += off;
        return *this;
    }

    inline size_t rest() const { return lim - offset; }
};
