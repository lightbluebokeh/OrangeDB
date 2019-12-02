#pragma once

#include <cstring>
#include <defs.h>

class File;
class SavedTable;

class Column {
private:
    String name;
    datatype_t type;
    int maxsize;
    int p = 18, s = 0;  // type == ORANGE_NUMERIC 才有效
    bool unique, nullable, index;
    byte_arr_t dft;
    std::vector<pred_t> ranges;
    bool is_string() const { return type == ORANGE_CHAR || type == ORANGE_VARCHAR; }

    bool test_size(byte_arr_t& val) const {
        if (int(val.size()) > maxsize) return 0;
        if (int(val.size()) < maxsize) {
            // 对于 null 或者字符串才补零
            if (val.front() != DATA_NULL && !is_string()) return 0;
            if (type != ORANGE_VARCHAR) val.resize(maxsize);
        }
        return 1;
    }
public:
    Column() {}
    Column(const String& name) : name(name), type(ORANGE_VARCHAR) {}
    Column(const String& name, datatype_t type, int value, bool unique, bool index, bool nullable, byte_arr_t dft, 
        std::vector<pred_t> ranges) : name(name), type(type), unique(unique), nullable(nullable), index(index), dft(dft), ranges(ranges) {
        switch (type) {
            case ORANGE_INT: maxsize = 4 + 1;
            break;
            case ORANGE_VARCHAR:
                maxsize = value + 1;
                ensure(maxsize <= MAX_VARCHAR_LEN, "varchar limit too long");
            break;
            case ORANGE_CHAR:
                maxsize = value + 1;
                ensure(maxsize <= MAX_CHAR_LEN, "char limit too long");
            break;
            case ORANGE_DATE:
                maxsize = 2 + 1;
            break;
            case ORANGE_NUMERIC:
                maxsize = 17 + 1;
                p = value / 40;
                s = value % 40;
                ensure(0 <= s && s <= p && p <= 20, "bad numeric");
            break;
        }
        for (auto &pred: this->ranges) {
            ensure(test_size(pred.lo) && test_size(pred.hi), "bad constraint parameter");
        }
        ensure(test(this->dft), "default value fails constraint");
    }

    String type_string() {
        switch (type) {
            case ORANGE_INT: return "int";
            case ORANGE_CHAR: return "char(" + std::to_string(maxsize - 1) + ")";
            case ORANGE_VARCHAR: return "varchar(" + std::to_string(maxsize - 1) + ")";
            case ORANGE_DATE: return "date";
            case ORANGE_NUMERIC: return "nummeric(" + std::to_string(p) + "," + std::to_string(s) + ")";
        }
        return "<error-type>";
    }

    // one more bytes for null/valid
    int key_size() const { return type == ORANGE_VARCHAR ? 1 + sizeof(size_t) : maxsize; }

    String get_name() const { return name; }
    bool has_dft() const { return nullable || dft[0]; }
    byte_arr_t get_dft() const { return dft; }

    // 测试 val 能否插入到这一列；对于 非 varchar 会补零
    bool test(byte_arr_t& val) const {
        if (val.empty()) return 0;
        // 只允许插入 null 或者普通的数据
        if (val.front() != DATA_NORMAL || val.front() != DATA_NULL) return 0;
        if (!test_size(val)) return 0;
        if (val.front() == DATA_NULL) return nullable;
        for (auto &pred: ranges) if (!pred.test(val, type)) return 0;
        return 1;
    }

    bool has_index() const { return index; }
    bool is_unique() const { return unique; }
    datatype_t get_datatype() const { return type; }

    friend class File;
    friend class SavedTable;
};

constexpr auto col_size = sizeof(Column);
