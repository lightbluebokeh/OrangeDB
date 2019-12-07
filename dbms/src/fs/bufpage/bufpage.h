#pragma once

#include "defs.h"
#include "fs/bufpage/bufpage_manage.h"

class File;

class Bufpage {
private:
    page_t page;
    buf_t buf;

public:
    Bufpage(page_t page) : page(page) {}
    Bufpage(int file_id, int page_id) : Bufpage(page_t{file_id, page_id}) {}

    void ensure_buf() {
        static_assert(sizeof(page_t) == sizeof(int64));
        if (buf.bytes == nullptr || *(int64*)&page != *(int64*)&(const page_t&)BufpageManage::get_page(buf.buf_id)) {
            buf = BufpageManage::get_page_buf(page);
        }
    }

    friend class File;
    friend class BufpageStream;
};