#pragma once

#include "orange/parser/sql_ast.h"

#include <cstring>

class File;
class SavedTable;

class Column {
private:
    String name;
    int id; // 所在 table 的列编号
    ast::data_type type;
    // orange_t type;
    // int maxsize;    // 对于非 varchar 就是正常的 size
    // int p = 18, s = 0;  // type == orange_t::Numeric 才有效
    bool nullable;
    ast::data_value dft;
    std::vector<std::pair<ast::op, ast::data_value>> checks;
    
    bool is_string() const { return type == orange_t::Char || type == orange_t::Varchar; }

    bool test_size(byte_arr_t& val) const {
        if (int(val.size()) > maxsize) return 0;
        if (int(val.size()) < maxsize) {
            // 对于 null 或者字符串才补零
            if (val.front() != DATA_NULL && !is_string()) return 0;
            if (type != orange_t::Varchar) val.resize(maxsize);
        }
        return 1;
    }
public:
    Column() {}
    Column(const String& name) : name(name), type(orange_t::Varchar) {}
    // type_value 对于 int 等类型无意义 
    Column(const String& name, int id, const ast::data_type& type, bool nullable, ast::data_value dft) : name(name), type(type), nullable(nullable), dft(dft) {
        switch (type.kind) {
            case orange_t::Varchar:
                orange_ensure(type.int_value() <= MAX_VARCHAR_LEN, "varchar limit too long");
            break;
            case orange_t::Char:
                orange_ensure(type.int_value() <= MAX_CHAR_LEN, "char limit too long");
            break;
            case orange_t::Date:
                maxsize = 2 + 1;
            break;
            case orange_t::Numeric:
                maxsize = 17 + 1;
                p = type_value / 40;
                s = type_value % 40;
                orange_ensure(0 <= s && s <= p && p <= 20, "bad numeric");
            break;
        }
        
        // for (auto &pred: this->ranges) {
        //     orange_ensure(test_size(pred.lo) && test_size(pred.hi), "bad constraint parameter");
        // }
    }

    String type_string() {
        switch (type.kind) {
            case orange_t::Int: return "int";
            case orange_t::Char: return "char(" + std::to_string(type.int_value()) + ")";
            case orange_t::Varchar: return "varchar(" + std::to_string(type.int_value()) + ")";
            case orange_t::Date: return "date";
            case orange_t::Numeric: return "nummeric(" + std::to_string(type.int_value() / 40) + "," + std::to_string(type.int_value() % 40) + ")";
        }
        return "<error-type>";
    }

    // one more bytes for null/valid
    int key_size() const { return type == orange_t::Varchar ? 1 + sizeof(size_t) : maxsize; }

    String get_name() const { return name; }
    ast::data_value get_dft() const {
        dft;
    }

    // 列完整性约束，返回是否成功和错误消息
    std::pair<bool, String> check(const ast::data_value& value) const {
        using ast::data_value_kind;
        switch (value.kind) {
            case data_value_kind::Null: return std::make_pair(nullable, "null value given to not null column"); // 懒了，都返回消息
            case data_value_kind::Int: return std::make_pair(type.kind == orange_t::Int || type.kind == orange_t::Numeric, "incompatible type");
            case data_value_kind::Float: return std::make_pair(type.kind == orange_t::Numeric, "incompatible type");
            case data_value_kind::String:
                switch (type.kind) {
                    case orange_t::Varchar:
                    case orange_t::Char: {
                        auto &str = value.to_string();
                        if (str.length() > type.int_value()) return std::make_pair(0, type.kind == orange_t::Char ? "char" : "varchar" + String("limit exceeded"));
                        return std::make_pair(0, "incompatible type");
                    } break;
                    case orange_t::Date: ORANGE_UNIMPL
                    default: std::make_pair(0, "incompatible type");
                }
            break;
        }
        return std::make_pair(1, "");
    }

    ast::data_type get_datatype() const { return type; }

    friend class SavedTable;
    friend std::istream& operator >> (std::istream& is, Column& col);
    friend std::ostream& operator << (std::ostream& os, const Column& col);
};

inline std::istream& operator >> (std::istream& is, Column& col) {
    is >> col.name;
    return is;
}

inline std::ostream& operator << (std::ostream& os, const Column& col) {
    os << col.name;
    return os;
}

constexpr auto col_size = sizeof(Column);
