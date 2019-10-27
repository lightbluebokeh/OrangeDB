#pragma once

#include <defs.h>

struct p_key_t {
    col_name_list_t list;
    bool valid;
};

// 我就看看
constexpr int p_key_size = sizeof(p_key_t);

struct f_key_t {
    f_key_name_t name;
    tbl_name_t ref_tbl;
    col_name_list_t list, ref_list;
};

constexpr size_t f_key_size = sizeof(f_key_t);