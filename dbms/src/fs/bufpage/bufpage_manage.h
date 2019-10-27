#pragma once

#include <defs.h>

namespace BufpageManage {
    void write_back();
    // 要确保 buf_id 被使用过
    page_t get_page(int buf_id);
    bool tracking(page_t page);
    void wirte_back_file(int file_id);
    buf_t get_page_buf(page_t page);
    void mark_dirty(int buf_id);
}