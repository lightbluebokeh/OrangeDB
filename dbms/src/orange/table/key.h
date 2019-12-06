#pragma once

#include <defs.h>

// foreign key type
struct f_key_t {
    String name;
    String ref_tbl;
    std::vector<String> list, ref_list;

    f_key_t() {}
    f_key_t(const String& name, const String& ref_tbl, const std::vector<String>& list, const std::vector<String>& ref_list) : name(name), ref_tbl(ref_tbl), list(list), ref_list(ref_list) {}
};


template<typename T>
std::enable_if_t<std::is_same_v<T, f_key_t>, std::ostream&> operator << (std::ostream& os, const T& t) {
    os << t.name << ' ' << t.ref_tbl << ' ' << t.list << ' ' << t.ref_list;
    return os;
}
template<typename T>
std::enable_if_t<std::is_same_v<T, f_key_t>, std::istream&> operator >> (std::istream& is, T& t) {
    is >> t.name >> t.ref_tbl >> t.list >> t.ref_list;
    return is;
}

// constexpr size_t f_key_size = sizeof(f_key_t);