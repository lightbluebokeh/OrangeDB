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
            auto &null = where.null_check();
            auto flag = get_field(get_col_id(null.col_name), rid).front();
            return null.is_not_null && flag != DATA_NULL || !null.is_not_null && flag == DATA_NULL;
        } else if (where.is_op()) {
            auto &op = where.op();
            auto col1_id = get_col_id(op.col_name);
            if (op.expression.is_column()) {
                auto &col2 = op.expression.col();
                orange_ensure(!col2.table_name.has_value(), "cannot specify table name here");
                int col2_id = get_col_id(col2.col_name);
                return Orange::cmp(get_field(col1_id, rid), cols[col1_id].get_datatype().kind, op.operator_, get_field(col2_id, rid), cols[col2_id].get_datatype().kind);
            } else if (op.expression.is_value()) {
                auto &value = op.expression.value();
                return Orange::cmp(get_field(col1_id, rid), cols[col1_id].get_datatype_kind(), op.operator_, value);
            } else {
                ORANGE_UNIMPL
                return 0;
            }
        } else {
            ORANGE_UNREACHABLE
            return 0;
        }
    }
protected:
    std::vector<Column> cols;

    virtual ~Table() {}

    const auto& get_col(const String& col_name) const {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        orange_ensure(it != cols.end(), "unknown column name: `" + col_name + "`");
        return *it;
    }
    auto get_cols(const std::vector<String> col_names) const {
        std::vector<Column> ret;
        for (auto &col_name: col_names) {
            ret.push_back(col_name);
        }
        return ret;
    }
    int get_col_id(const String& col_name) const { 
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        orange_ensure(it != cols.end(), "unknown column name: `" + col_name + "`");
        return it - cols.begin(); 
    }
    bool has_col(const String& col_name) const { 
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        return it != cols.end();
    }

    virtual byte_arr_t get_field(int col_id, rid_t rid) const = 0;

    // 返回所有合法 rid
    virtual std::vector<rid_t> all() const = 0;
    // default brute force
    virtual std::vector<rid_t> single_where(const ast::single_where& where) const {
        std::vector<rid_t> ret;
        for (auto rid: all()) {
            if (check_where(rid, where)) ret.push_back(rid);
        }
        return ret;
    }
    // 用 where 进行筛选
    virtual std::vector<rid_t> filt(const std::vector<rid_t>& rids, const ast::single_where& where) const {
        std::vector<rid_t> ret;
        for (auto rid: rids) {
            if (check_where(rid, where)) ret.push_back(rid);
        }
        return ret;
    }
    // 返回满足 where clause 的所有 rid
    virtual std::vector<rid_t> where(const ast::where_clause& where, rid_t lim = MAX_RID) const {
        // 多条 where clause 是与关系，求交
        auto ret = all();
        for (auto &single_where: where) ret = filt(ret, single_where);
        return ret;
    }
public:
    virtual TmpTable select(std::vector<String> names, const std::vector<rid_t>& rids) const;
    TmpTable select_star(const std::vector<rid_t>& rids) const;
};

class SavedTable;

class TmpTable : public Table {
private:
    std::vector<rec_t> recs;
    std::vector<rid_t> all() const {
        std::vector<rid_t> ret;
        for (rid_t i = 0; i < recs.size(); i++) {
            ret.push_back(i);
        }
        return ret;
    }
public:
    byte_arr_t get_field(int col_id, rid_t rid) const override { return recs[rid][col_id]; }

    static TmpTable from_strings(const String& title, const std::vector<String>& strs) {
        TmpTable ret;
        ret.cols.push_back(Column(title));
        ret.recs.reserve(strs.size());
        for (auto &str: strs) {
            ret.recs.push_back({Orange::to_bytes(str)});
        }
        return ret;
    }

    friend class Table;
    friend class SavedTable;
    friend std::ostream& operator << (std::ostream& os, const TmpTable& table);
};

inline TmpTable Table::select(std::vector<String> names, const std::vector<rid_t>& rids) const {
    TmpTable ret;
    ret.recs.resize(rids.size());
    for (auto name: names) {
        auto col_id = get_col_id(name);
        ret.cols.push_back(cols[col_id]);
        for (auto rid: rids) {
            ret.recs[rid].push_back(get_field(rid, col_id));
        }
    }
    return ret;          
}

inline TmpTable Table::select_star(const std::vector<rid_t>& rids) const {
    std::vector<String> names;
    for (auto &col: cols) names.push_back(col.get_name());
    return select(names, rids);
}


inline std::ostream& operator << (std::ostream& os, const TmpTable& table) {
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
    auto print = [&] (const String& str) {
        if (str.length() + 3 > width) {
            os << str.substr(0, width - 3) + "...";
        } else {
            os << str;
            for (unsigned i = 0; i + str.length() < width; i++) {
                os.put(' ');
            }
        }
    };
    auto print_line = [&] (const std::vector<String>& data) {
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

    auto to_string = [] (const byte_arr_t &bytes, orange_t type) -> String {
        if (bytes.front() == DATA_NULL) return "null";
        switch (type) {
            case orange_t::Varchar:
            case orange_t::Char: return Orange::bytes_to_string(bytes);
            case orange_t::Int: return std::to_string(Orange::bytes_to_int(bytes));
            case orange_t::Numeric:
            case orange_t::Date: ORANGE_UNIMPL
            default: ORANGE_UNIMPL
        }
        return "fuck warning";
    };

    for (auto &rec: table.recs) {
        for (unsigned i = 0; i < data.size(); i++) data[i] = to_string(rec[i], table.cols[i].get_datatype().kind);
        // 打印表的每一列
        print_line(data);
    }

    // 下边线
    print_divide();
    return os;
}
