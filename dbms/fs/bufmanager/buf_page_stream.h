#pragma once

#include <fs/bufmanager/buf_page.h>

class BufPageStream {
private:
    BufPage page;
    size_t offset;
    BufPageStream(const BufPageStream&) = delete;
public:
    BufPageStream(const BufPage& page) : page(page), offset(0) {}

    // 默认为 n = vec.size();
    template<typename T>
    BufPageStream& write_bytes(const std::vector<T>& vec) {
        offset += page.write_bytes(vec, offset, vec.size());
        return *this;
    }

    template<typename T>
    BufPageStream& write_bytes(const std::basic_string<T>& str, size_t n = 0) {
        offset += page.write_bytes(str, offset, n);
        return *this;
    }

    template<typename T>
    BufPageStream& write_obj(const T& t, size_t n = sizeof(T)) {
        offset += page.write_obj(t, offset, n);
        return *this;
    }

    BufPageStream& memset(byte_t c = 0) {
        offset += page.memset(c, offset);
        return *this;
    }

    BufPageStream& memset(byte_t c, size_t n) {
        offset += page.memset(c, offset, n);
        return *this;
    }

    template<typename T>
    const T& get() {
        const T& ret = page.get<T>(offset);
        offset += sizeof(T);
        return ret;
    }

    // 读取 sizeof(T) 个字节传给 U
    template<typename T, typename U = T>
    BufPageStream& get_obj(U& u) {
        offset += page.get_obj<T, U>(u, offset);
        return *this;
    }

    template<typename T>
    BufPageStream& get_bytes(std::vector<T>& vec, size_t n) {
        offset += page.get_bytes(vec, n, offset);
        return *this;
    }

    template<typename T>
    BufPageStream& get_bytes(std::basic_string<T>& str, size_t n) {
        offset += page.get_bytes(str, n, offset);
        return *this;
    }

    BufPageStream& inc(size_t n) {
        offset += n;
        return *this;
    }
};