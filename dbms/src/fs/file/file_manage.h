#pragma once

#include <defs.h>

namespace FileManage {
    int write_page(page_t page, bytes_t bytes, int off = 0);
    int read_page(page_t page, bytes_t bytes, int off = 0);
    int close_file(int file_id);
    int create_file(const String& name);
    // return fd if success
    int open_file(const String& name, int& file_id, int& fd);
    int remove_file(const String& name);
    bool file_opened(const String& name);
    bool file_exists(const String& name);
}  // namespace FileManage
