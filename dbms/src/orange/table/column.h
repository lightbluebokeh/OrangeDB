#pragma once

#include "fs/allocator/allocator.h"
#include "orange/parser/sql_ast.h"

#include <cstring>
#include <utility>

class File;
class SavedTable;

class Column {
private:
    String name;
    // int id; // 所在 table 的列编号
    ast::data_type type;
    bool nullable;
    ast::data_value dft;
    std::vector<std::pair<ast::op, ast::data_value>> checks;

public:
    explicit Column() = default;
    // 适合打印名称的那种
    explicit Column(String name) :
        name(std::move(name)), type{orange_t::Varchar, MAX_VARCHAR_LEN} {}
    Column(String name, const ast::data_type& type, bool nullable, ast::data_value dft) : // int id
        name(std::move(name)), type(type), nullable(nullable), dft(std::move(dft)) {
        switch (type.kind) {
            case orange_t::Int:
                // do nothing
                break;
            case orange_t::Varchar:
                orange_check(type.int_value() <= MAX_VARCHAR_LEN, "varchar limit too long");
                break;
            case orange_t::Char:
                orange_check(type.int_value() <= MAX_CHAR_LEN, "char limit too long");
                break;
            case orange_t::Date: break;
            case orange_t::Numeric: {
                int p = type.int_value() / 40;
                int s = type.int_value() % 40;
                orange_check(0 <= s && s <= p && p <= 20, "bad numeric");
            } break;
        }
    }

    // int get_id() const { return id; }

    [[nodiscard]] String type_string() const {
        switch (type.kind) {
            case orange_t::Int: return "int";
            case orange_t::Char: return "char(" + std::to_string(type.int_value()) + ")";
            case orange_t::Varchar: return "varchar(" + std::to_string(type.int_value()) + ")";
            case orange_t::Date: return "date";
            case orange_t::Numeric:
                return "numeric(" + std::to_string(type.int_value() / 40) + "," +
                       std::to_string(type.int_value() % 40) + ")";
        }
        return "<error-type>";
    }

    [[nodiscard]] int get_key_size() const { return type.key_size(); }

    [[nodiscard]] String get_name() const { return name; }
    [[nodiscard]] ast::data_value get_dft() const { return dft; }
    [[nodiscard]] bool is_nullable() const { return nullable; }

    // 列完整性约束，返回是否成功和错误消息
    [[nodiscard]] std::pair<bool, String> check(const ast::data_value& value) const {
        using ast::data_value_kind;
        switch (value.kind()) {
            case data_value_kind::Null:
                return std::make_pair(nullable, "column constraint failed: null value given to not "
                                                "null column");  // 懒了，都返回消息
            case data_value_kind::Int:
                return std::make_pair(type.kind == orange_t::Int || type.kind == orange_t::Numeric,
                                      "incompatible type");
            case data_value_kind::Float:
                return std::make_pair(type.kind == orange_t::Numeric,
                                      "column constraint failed: incompatible type");
            case data_value_kind::String:
                switch (type.kind) {
                    case orange_t::Varchar:
                    case orange_t::Char: {
                        auto& str = value.to_string();
                        if (int(str.length()) > type.int_value())
                            return std::make_pair(
                                0, String("column constraint failed: ") +
                                       (type.kind == orange_t::Char ? "char" : "varchar") +
                                       String("limit exceeded"));
                        return std::make_pair(1, "");
                    }
                    case orange_t::Date: break;
                    default:
                        return std::make_pair(0, "column constraint failed: incompatible type");
                }
                break;
        }
        return std::make_pair(1, "");
    }

    [[nodiscard]] ast::data_type get_datatype() const { return type; }
    [[nodiscard]] orange_t get_datatype_kind() const { return type.kind; }

    static int key_size_sum(const std::vector<Column>& cols) {
        int ret = 0;
        for (auto& col : cols) ret += col.get_key_size();
        return ret;
    }

    static Column from_def(const ast::field_def& def) {
        return Column(def.col_name, def.type, !def.is_not_null,
                      def.default_value.get_value_or(ast::data_value::null_value()));
    }

    friend std::istream& operator>>(std::istream& is, Column& col);
    friend std::ostream& operator<<(std::ostream& os, const Column& col);

    friend class SavedTable;
};

inline std::ostream& operator<<(std::ostream& os, const Column& col) {
    // print(os, ' ', col.name, col.id, col.type, col.nullable, col.dft, col.checks, col.key_size);
    os << col.name << ' ' <<
        // col.id << ' ' <<
        col.type << ' ' << col.nullable << ' ' << col.dft << ' ' << col.checks;
    return os;
}
inline std::istream& operator>>(std::istream& is, Column& col) {
    is >> col.name >>
        // col.id >>
        col.type >> col.nullable >> col.dft >> col.checks;
    return is;
}
