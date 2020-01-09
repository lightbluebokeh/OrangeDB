#pragma once

#include <defs.h>

#include <utility>

// foreign key type
struct f_key_t {
    String name;
    String ref_tbl;
    std::vector<String> list, ref_list;

    f_key_t() = default;
    f_key_t(String name, String ref_tbl, std::vector<String> list, std::vector<String> ref_list) :
        name(std::move(name)), ref_tbl(std::move(ref_tbl)), list(std::move(list)),
        ref_list(std::move(ref_list)) {}
};


template <typename T>
std::enable_if_t<std::is_same_v<T, f_key_t>, std::ostream&> operator<<(std::ostream& os,
                                                                       const T& t) {
    os << t.name << ' ' << t.ref_tbl << ' ' << t.list << ' ' << t.ref_list;
    return os;
}
template <typename T>
std::enable_if_t<std::is_same_v<T, f_key_t>, std::istream&> operator>>(std::istream& is, T& t) {
    is >> t.name >> t.ref_tbl >> t.list >> t.ref_list;
    return is;
}

// constexpr size_t f_key_size = sizeof(f_key_t);