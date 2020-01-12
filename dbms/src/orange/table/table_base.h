#pragma once

#include <set>

#include "defs.h"
#include "orange/common.h"
#include "orange/table/column.h"

class TmpTable;

class Table {
private:
    // 慢是慢了点，不过用暴力的话已经在内存里了，真慢了再说
    bool check_where(rid_t rid, const ast::single_where& where) const {
        if (where.is_null_check()) {
            auto& null = where.null_check();
            auto flag = get_field(get_col_id(null.col.col_name), rid).front();
            return (null.is_not_null && flag != DATA_NULL) ||
                   (!null.is_not_null && flag == DATA_NULL);
        } else if (where.is_op()) {
            auto& op = where.op();
            auto col1_id = get_col_id(op.col.col_name);
            auto& expr = op.expression;
            if (expr.is_column()) {
                auto& col2 = expr.col();
                orange_check(!col2.table_name.has_value(), "cannot specify table name here");
                int col2_id = get_col_id(col2.col_name);
                return Orange::cmp(get_field(col1_id, rid), cols[col1_id].get_datatype().kind,
                                   op.operator_, get_field(col2_id, rid),
                                   cols[col2_id].get_datatype().kind);
            } else if (expr.is_value()) {
                auto& value = expr.value();
                return Orange::cmp(get_field(col1_id, rid), cols[col1_id].get_datatype_kind(),
                                   op.operator_, value);
            } else {
                ORANGE_UNREACHABLE
                return 0;
            }
        } else {
            ORANGE_UNREACHABLE
        }
    }

protected:
    std::vector<Column> cols;

    virtual ~Table() = default;

    auto& get_col(const String& col_name) {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        orange_check(it != cols.end(), Exception::col_not_exist(col_name, get_name()));
        return *it;
    }
    const auto& get_col(const String& col_name) const {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        orange_check(it != cols.end(), Exception::col_not_exist(col_name, get_name()));
        return *it;
    }
    auto get_cols(const std::vector<String>& col_names) const {
        std::vector<Column> ret;
        for (auto& col_name : col_names) {
            ret.push_back(get_col(col_name));
        }
        return ret;
    }
    int get_col_id(const String& col_name) const {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        orange_check(it != cols.end(), Exception::col_not_exist(col_name, get_name()));
        return it - cols.begin();
    }
    int get_col_id(const Column& col) const { return get_col_id(col.get_name()); }
    std::vector<int> get_col_ids(const std::vector<String>& col_names) const {
        std::vector<int> ret;
        for (auto& col_name : col_names) {
            ret.push_back(get_col_id(col_name));
        }
        return ret;
    }
    bool has_col(const String& col_name) const {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        return it != cols.end();
    }

    [[nodiscard]] virtual byte_arr_t get_field(int col_id, rid_t rid) const = 0;
    auto get_fields(const std::vector<Column>& cols, rid_t rid) const {
        std::vector<byte_arr_t> ret;
        for (auto& col : cols) ret.push_back(get_field(get_col_id(col.get_name()), rid));
        return ret;
    }

    // 返回所有合法 rid
    virtual std::vector<rid_t> all() const = 0;
    // 用 where 进行筛选
    virtual std::vector<rid_t> filt(const std::vector<rid_t>& rids,
                                    const ast::single_where& where) const {
        std::vector<rid_t> ret;
        for (auto rid : rids) {
            if (check_where(rid, where)) ret.push_back(rid);
        }
        return ret;
    }
    // where 中含有和 null 的比较
    bool check_op_null(const ast::where_clause& where_clause) const {
        for (auto& where : where_clause) {
            if (where.is_op()) {
                auto& expr = where.op().expression;
                if (expr.is_value() && expr.value().is_null()) return true;
            }
        }
        return false;
    }

public:
    virtual String get_name() const { return ANONYMOUS_NAME; }
    [[nodiscard]] const std::vector<Column>& get_cols() const { return cols; }

    // 返回满足 where clause 的所有 rid
    // 为了测试放到了 public
    virtual std::vector<rid_t> where(const ast::where_clause& where, rid_t lim = MAX_RID) const {
        if (check_op_null(where)) return {};
        // 多条 where clause 是与关系，求交
        auto ret = all();
        for (auto& single_where : where) ret = filt(ret, single_where);
        if (ret.size() > lim) ret.resize(lim);
        return ret;
    }
    virtual TmpTable select(const std::vector<String>& names,
                            const ast::where_clause& where, rid_t lim = MAX_RID) const = 0;
    TmpTable select_star(const ast::where_clause& where, rid_t lim = MAX_RID) const;
};

class SavedTable;

class TmpTable : public Table {
private:
    std::vector<rec_t> recs;
    [[nodiscard]] std::vector<rid_t> all() const override {
        std::vector<rid_t> ret;
        for (rid_t i = 0; i < recs.size(); i++) {
            ret.push_back(i);
        }
        return ret;
    }

public:
    byte_arr_t get_field(int col_id, rid_t rid) const override { return recs[rid][col_id]; }
    const std::vector<rec_t>& get_recs() const { return recs; }

    static TmpTable from_strings(const String& title, const std::vector<String>& strs) {
        TmpTable ret;
        ret.cols.emplace_back(title);
        ret.recs.reserve(strs.size());
        for (auto& str : strs) {
            ret.recs.push_back({Orange::to_bytes(str)});
        }
        return ret;
    }

    [[nodiscard]] TmpTable select(const std::vector<String>& names,
                                  const ast::where_clause& where, rid_t lim) const override {
        TmpTable ret;
        auto rids = this->where(where, lim);
        auto col_ids = get_col_ids(names);
        ret.recs.resize(rids.size());
        for (auto col_id : col_ids) {
            ret.cols.push_back(cols[col_id]);
            for (auto rid : rids) {
                ret.recs[rid].push_back(get_field(rid, col_id));
            }
        }
        return ret;
    }

    friend class Table;
    friend class SavedTable;
    friend std::ostream& operator << (std::ostream& os, const TmpTable& table);
};

inline TmpTable Table::select_star(const ast::where_clause& where, rid_t lim) const {
    std::vector<String> names;
    for (auto& col : cols) names.push_back(col.get_name());
    return select(names, where, lim);
}


inline std::ostream& operator<<(std::ostream& os, const TmpTable& table) {
    using std::endl;

    // 最后三个字符留作省略号
    constexpr int width = 12 + 3;

    // 分割线
    auto print_divide = [&] {
        os.put('+');
        for (unsigned i = 0; i < table.cols.size(); i++) {
            for (int j = 0; j < width; j++) os.put('-');
            if (i + 1 < table.cols.size()) os.put('+');
        }
        os.put('+') << endl;
    };
    // 带截断的打印字符串
    auto print = [&](const String& str) {
        if (int(str.length()) + 3 > width) {
            os << str.substr(0, width - 3) + "...";
        } else {
            os << str;
            for (int i = 0; int(i + str.length()) < width; i++) {
                os.put(' ');
            }
        }
    };
    auto print_line = [&](const std::vector<String>& data) {
        os.put('|');
        for (unsigned i = 0; i < data.size(); i++) {
            print(data[i]);
            if (i + 1 < data.size()) os.put('|');
        }
        os.put('|') << endl;
    };

    std::vector<String> data;
    data.resize(table.cols.size());

    // 上边线
    print_divide();

    for (unsigned i = 0; i < data.size(); i++) data[i] = table.cols[i].get_name();
    // 打印列名
    print_line(data);

    // 中间线
    print_divide();

    auto to_string = [] (const byte_arr_t& bytes, orange_t type) -> String {
        if (bytes.front() == DATA_NULL) return "null";
        switch (type) {
            case orange_t::Varchar:
            case orange_t::Char: return Orange::bytes_to_string(bytes);
            case orange_t::Int: return std::to_string(Orange::bytes_to_int(bytes));
            case orange_t::Numeric: return std::to_string(Orange::bytes_to_numeric(bytes));
            case orange_t::Date: {
                std::tm date = {};
                memcpy(&date, 1 + bytes.data(), sizeof(std::tm));
                std::ostringstream ss;
                ss << std::put_time(&date, "%Y-%m-%d");
                return ss.str();
            }
            default: return "<error>???";
        }
    };

    for (auto& rec : table.recs) {
        for (unsigned i = 0; i < data.size(); i++)
            data[i] = to_string(rec[i], table.cols[i].get_datatype().kind);
        // 打印表的每一列
        print_line(data);
    }

    // 下边线
    print_divide();

    os << "total: " << table.recs.size() << endl;
    return os;
}
