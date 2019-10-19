#pragma once

#include <defs.h>
#include <fs/bufmanager/BufPageManager.h>

class File;
class BufPageOstream;
class BufPage {
private:
    page_t page;
    buf_t buf;

    BufPage(page_t page) : page(page) {}
    BufPage(int file_id, int page_id) : BufPage(page_t{file_id, page_id}) {}
    BufPage(const BufPage&) = default;

    // 读写操作前调用
    void check_overflow(size_t offset, size_t n) { ensure(offset + n <= PAGE_SIZE, "page IO overflow"); }
    void ensure_buf() {
        auto bfm = BufPageManager::get_instance();
        if (buf.bytes == nullptr || std::not_equal_to<page_t>()(page, bfm->get_page(buf.buf_id))) {
            buf = bfm->get_page_buf(page);
        }
    }
public:
    template<typename T>
    T get(int offset = 0) {
        check_overflow(offset, sizeof(T));
        ensure_buf();
        return *(T*)buf.bytes;
    }

    template<typename T>
    size_t get_obj(T& t, size_t offset = 0) {
        check_overflow(offset, sizeof(T));
        ensure_buf();
        t = *(T*)(buf.bytes + offset);
        return sizeof(T);
    }

    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> get_bytes(std::vector<T>& vec, size_t n, size_t offset = 0) {
        check_overflow(offset, n);
        ensure_buf();
        vec = std::vector<T>(buf.bytes + offset, buf.bytes + offset + n);
        return n;
    }

    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> get_bytes(std::basic_string<T>& str, size_t n, size_t offset = 0) {
        check_overflow(offset, n);
        ensure_buf();
        str = std::basic_string<T>(buf.bytes + offset, buf.bytes + offset + n);
        return n;
    }

    // 默认为 n = vec.size();
    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(const std::vector<T>& vec, size_t offset = 0) {
        return write_bytes(vec, offset, vec.size());
    }

        template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(const std::vector<T>& vec, size_t offset, size_t n) {
        check_overflow(offset, n);
        ensure_buf();
        size_t ret = BytesIO::write_bytes(buf.bytes + offset, vec, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return ret;
    }

    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(const std::basic_string<T>& str, size_t offset = 0) {
        return write_bytes(str, offset, str.size());
    }

    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(const std::basic_string<T>& str, size_t offset, size_t n) {
        check_overflow(offset, n);
        ensure_buf();
        size_t ret = BytesIO::write_bytes(buf.bytes + offset, str, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return ret;     
    }

    template<typename T>
    size_t write_obj(const T& t, int offset = 0, size_t n = sizeof(T)) {
        check_overflow(offset, n);
        ensure_buf();
        size_t ret = BytesIO::write_obj(buf.bytes + offset, t, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return ret;
    }

    size_t memset(byte_t c, size_t offset = 0) { return memset(c, offset, PAGE_SIZE - offset); }

    size_t memset(byte_t c, size_t offset, size_t n) {
        check_overflow(offset, n);
        ensure_buf();
        ::memset(buf.bytes + offset, c, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return n;
    }

    friend class File;
    friend class BufPageStream;
    friend int main();  //  for test
};