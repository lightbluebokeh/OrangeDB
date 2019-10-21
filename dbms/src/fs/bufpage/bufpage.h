#pragma once

#include <defs.h>
#include <fs/bufpage/bufpage_manager.h>

class File;

class Bufpage {
private:
    page_t page;
    buf_t buf;

public:
    Bufpage(page_t page) : page(page) {}
    Bufpage(int file_id, int page_id) : Bufpage(page_t{file_id, page_id}) {}

    void ensure_buf() {
        auto bfm = BufpageManager::get_instance();
        if (buf.bytes == nullptr || std::not_equal_to<page_t>()(page, bfm->get_page(buf.buf_id))) {
            buf = bfm->get_page_buf(page);
        }
    }

    friend class BufpageStream;
};