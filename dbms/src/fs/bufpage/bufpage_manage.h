#pragma once

#include <mutex>
#include <string>
#include <cstring>
#include <cassert>

#include <fs/file/file_manage.h>
#include <fs/bufpage/bufpage_manage.h>
#include <utils/YourLinkList.h>
#include <utils/find_replace.h>
#include <fs/bufpage/bufpage_map.h>


class Bufpage;
class BufpageStream;

namespace BufpageManage {
    void write_back();
    // 要确保 buf_id 被使用过
    page_t get_page(int buf_id);
    bool tracking(page_t page);
    buf_t get_page_buf(page_t page);
    void mark_dirty(int buf_id);
}