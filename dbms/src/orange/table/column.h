#pragma once

#include <cstring>
#include <defs.h>

class File;

class col_t {
private:
    struct datatype_t {
        enum kind_t {
            INT,
            CHAR,
            VARCHAR,
            NUMERIC,
            DATE,
        };
        kind_t kind;
        int size;
        int p = 18, s = 0;  // kind == NUMERIC 才有效

        // 保证以 0 结尾 233
        static datatype_t parse(const String& raw_type) {
            int size, p, s;
            if (raw_type == "INT") {
                return {INT, 4};
            } else if (sscanf(raw_type.data(), "VARCHAR(%d)", &size) == 1) {
                ensure(size <= MAX_CHAR_LEN, "varchar limit too long");
                return {VARCHAR, size + 1};
            } else if (sscanf(raw_type.data(), "CHAR(%d)", &size) == 0) {
                ensure(size <= MAX_CHAR_LEN, "char limit too long");
                return {CHAR, size + 1};
            } else if (strcmp(raw_type.data(), "DATE") == 0) {
                return {DATE, 2};
            } else if (strcmp(raw_type.data(), "NUMERIC") == 0) {
                return {NUMERIC, 17};
            } else if (sscanf(raw_type.data(), "NUMERIC(%d)", &p) == 1) {
                return {NUMERIC, 17, p};
            } else if (sscanf(raw_type.data(), "NUMERIC(%d,%d)", &p, &s) == 2) {
                return {NUMERIC, 17, p, s};
            } else {
                throw "ni zhe shi shen me dong xi";
            }
        }

        bool is_string() const { return kind == CHAR || kind == VARCHAR; }
    };
    // col_name_t name;
    String name;
    datatype_t datatype;
    bool unique, nullable, index;
    byte_arr_t dft;
    std::vector<std::pair<byte_arr_t, byte_arr_t>> ranges;

    bool test_size(byte_arr_t& val) {
        unsigned size = get_size();
        if (val.size() > size) return 0;
        if (val.size() < size) {
            if (!datatype.is_string()) return 0;
            if (datatype.kind == datatype_t::CHAR) val.resize(size);
        }
        return 1;
    }
public:
    col_t() {}
    col_t(const String& name, const String& raw_type, bool unique, bool index, bool nullable, byte_arr_t dft, 
        std::vector<std::pair<byte_arr_t, byte_arr_t>> ranges) 
        : name(name), datatype(datatype_t::parse(raw_type)), 
            unique(unique), nullable(nullable), index(index), 
            dft(dft), ranges(ranges) {
        for (auto &[lo, hi]: this->ranges) {
            ensure(test_size(lo) && test_size(hi), "bad constraint parameter");
            lo.resize(get_size());
            hi.resize(get_size());
        }
        ensure(test(this->dft), "default value fails constraint");
    }

    // one more bytes for null/valid
    int get_size() { return 1 + datatype.size; }
    String get_name() { return name; }
    bool has_dft() { return nullable || dft[0]; }
    byte_arr_t get_dft() { return dft; }

    // 测试 val 能否插入到这一列；对于 char 会补零
    bool test(byte_arr_t& val) {
        if (val.empty()) return 0;
        if (!test_size(val)) return 0;
        if (!val.front()) return nullable;
        auto tmp = val;
        tmp.resize(get_size());
        for (auto [lo, hi]: ranges) {
            if (bytesncmp(tmp.data(), lo.data(), tmp.size()) < 0 
                || bytesncmp(hi.data(), tmp.data(), tmp.size())< 0) return 0;
        }
        return 1;
    }

    bool has_index() { return index; }
    bool is_unique() { return unique; }
    key_kind_t key_kind() {
        switch (datatype.kind) {
            case datatype_t::NUMERIC: return key_kind_t::NUMERIC;
            case datatype_t::VARCHAR: return key_kind_t::VARCHAR;
            default: return key_kind_t::BYTES;
        }
    }

    void write(File* file) const {
        file->seek_pos(0)->write(name, datatype, unique, nullable, index, dft, ranges);
    }

    void read(File* file) {
        file->seek_pos(0)->read(name, datatype, unique, nullable, index, dft, ranges);
    }
};

constexpr auto col_size = sizeof(col_t);
