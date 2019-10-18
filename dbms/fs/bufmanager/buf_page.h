#pragma once

#include <defs.h>
#include <fs/bufmanager/BufPageManager.h>


class File;
class BufPage {
private:
    page_t page;
    buf_t buf;

    // 读写操作前调用
    void ensure_buf() {
        // assert(FileManager::get_instance()->)
        auto bfm = BufPageManager::get_instance();
        if (buf.bytes == nullptr || std::not_equal_to<page_t>()(page, bfm->get_page(buf.buf_id))) {
            buf = bfm->get_page_buf(page);
        }
    }
    BufPage(page_t page) : page(page) {}
    BufPage(int file_id, int page_id) : BufPage(page_t{file_id, page_id}) {}
    BufPage(const BufPage&) = delete;
public:
    // const bytes_t get_bytes() { return buf.bytes; }
    template<typename T>
    const T& get(size_t offset = 0) {
        ensure_buf();
        return *(T*)(buf.bytes + offset);
    }

    // 默认为 n = vec.size();
    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(const std::vector<T>& vec, size_t offset = 0, size_t n = 0) {
        if (n == 0) n = vec.size();
        assert(offset + n <= PAGE_SIZE);
        ensure_buf();
        size_t ret = BytesIO::write_bytes(buf.bytes, vec, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return ret;
    }

    template<typename T>
    std::enable_if_t<is_byte_v<T>, size_t> write_bytes(const std::basic_string<T>& str, size_t offset = 0, size_t n = 0) {
        if (n == 0) n = str.size();
        assert(offset + n <= PAGE_SIZE);
        ensure_buf();
        size_t ret = BytesIO::write_bytes(buf.bytes, str, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return ret;     
    }

    template<typename T>
    size_t write_obj(const T& t, int offset = 0, size_t n = sizeof(T)) {
        assert(offset + n <= PAGE_SIZE);
        ensure_buf();
        size_t ret = BytesIO::write_obj(buf.bytes + offset, t, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return ret;
    }

    size_t memset(byte_t c, size_t offset = 0, size_t n = PAGE_SIZE) {
        assert(offset + n <= PAGE_SIZE);
        ensure_buf();
        ::memset(buf.bytes + offset, c, n);
        BufPageManager::get_instance()->mark_dirty(buf.buf_id);
        return n;
    }

    friend class File;
    friend int main();  //  for test
};