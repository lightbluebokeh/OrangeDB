#pragma once

#include <defs.h>

// struct p_key_t {
//     col_name_list_t list;
//     bool valid;
// };

// // 我就看看
// constexpr int p_key_size = sizeof(p_key_t);

struct f_key_t {
    String name;
    String ref_tbl;
    std::vector<String> list, ref_list;

    f_key_t() {}
    f_key_t(const String& name, const String& ref_tbl, const std::vector<String>& list, const std::vector<String>& ref_list) : name(name), ref_tbl(ref_tbl), list(list), ref_list(ref_list) {}
};

// constexpr size_t f_key_size = sizeof(f_key_t);