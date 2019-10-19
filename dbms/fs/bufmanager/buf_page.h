#pragma once

#include <defs.h>
#include <fs/bufmanager/BufPageManager.h>

class File;

class BufPage {
private:
    page_t page;
    buf_t buf;

public:
    BufPage(page_t page) : page(page) {}
    BufPage(int file_id, int page_id) : BufPage(page_t{file_id, page_id}) {}

    void ensure_buf() {
        auto bfm = BufPageManager::get_instance();
        if (buf.bytes == nullptr || std::not_equal_to<page_t>()(page, bfm->get_page(buf.buf_id))) {
            buf = bfm->get_page_buf(page);
        }
    }

    friend class BufPageStream;
};