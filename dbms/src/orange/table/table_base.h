#pragma once

#include <set>

#include "defs.h"
#include "orange/common.h"
#include "orange/table/column.h"

class TmpTable;

class Table {
protected:
    std::vector<Column> cols;

    virtual ~Table() {}

    auto get_col(const String& col_name) const {
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

    // 返回所有合法 rid
    virtual std::vector<rid_t> all() const = 0;
    virtual byte_arr_t get_field(rid_t rid, int col_id) const = 0;

    // 慢是慢了点，不过用暴力的话已经在内存里了，真慢了再说
    bool check_where(const Orange::parser::single_where& where, rid_t rid) const {
        if (where.is_null_check()) {
            auto &null = where.null_check();
            auto flag = get_field(rid, get_col_id(null.col_name)).front();
            return null.is_not_null && flag != DATA_NULL || !null.is_not_null && flag == DATA_NULL;
        } else if (where.is_op()) {
            auto &op = where.op();
            auto col1_id = get_col_id(op.col_name);
            // auto &col1 = cols[col1_id];
            if (op.expression.is_column()) {
                auto &col2 = op.expression.col();
                orange_ensure(!col2.table_name.has_value(), "cannot specify table name here");
                int col2_id = get_col_id(col2.col_name);
                return Orange::cmp(get_field(rid, col1_id), cols[col1_id].get_datatype(), op.operator_, get_field(rid, col2_id), cols[col2_id].get_datatype());
            } else if (op.expression.is_value()) {
                auto &value = op.expression.value();
                return Orange::cmp(get_field(rid, col1_id), cols[col1_id].get_datatype(), op.operator_, Orange::to_bytes(value), value.get_datatype());
            } else {
                ORANGE_UNIMPL
                return 0;
            }
        } else {
            ORANGE_UNIMPL
            return 0;
        }
    }

    // default brute force
    virtual std::vector<rid_t> single_where(const Orange::parser::single_where& where, rid_t lim = std::numeric_limits<rid_t>::max()) const {
        std::vector<rid_t> ret;
        for (auto rid: all()) {
            if (ret.size() >= lim) break;
            if (check_where(where, rid)) ret.push_back(rid);
        }
        return ret;
    }
public:
    virtual std::vector<rid_t> where(const Orange::parser::where_clause& where) const {
        // 多条 where clause 是与关系，求交
        if (where.empty()) return all();
        std::vector<rid_t> ret = single_where(where.front());
        for (auto i = 1u; i < where.size(); i++) {
            std::set<rid_t> set(ret.begin(), ret.end());
            ret.clear();
            auto cur = single_where(where[i]);
            for (auto rid: cur) {
                if (set.count(rid)) {
                    ret.push_back(rid);
                }
            }
        }
        return ret;
    }

    virtual TmpTable select(std::vector<String> names, const std::vector<rid_t>& rids) const;
    TmpTable select_star(const std::vector<rid_t>& rids) const;
};

class SavedTable;

class TmpTable : public Table {
private:
    std::vector<rec_t> recs;
public:
    std::vector<rid_t> all() const {
        std::vector<rid_t> ret;
        for (rid_t i = 0; i < recs.size(); i++) {
            ret.push_back(i);
        }
        return ret;
    }

    byte_arr_t get_field(rid_t rid, int col_id) const override { return recs[rid][col_id]; }

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

static String to_string(const byte_arr_t &bytes, orange_t type) {
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
}

// 最后三个字符留作省略号
constexpr static int width = 12 + 3;

inline std::ostream& operator << (std::ostream& os, const TmpTable& table) {
    using std::endl;

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

    for (auto &rec: table.recs) {
        for (unsigned i = 0; i < data.size(); i++) data[i] = ::to_string(rec[i], table.cols[i].get_datatype());
        // 打印表的每一列
        print_line(data);
    }

    // 下边线
    print_divide();
    return os;
}
