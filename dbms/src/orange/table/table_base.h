#pragma once

#include <defs.h>
#include <orange/table/column.h>

class TmpTable;

class Table {
protected:
    std::vector<Column> cols;

    std::vector<Column>::iterator find_col(const String& col_name) {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        ensure(it != cols.end(), "unknown column name: `" + col_name + "`");
        return it;
    }
    bool has_col(const String& col_name) { return find_col(col_name) != cols.end(); }
    virtual ~Table() {}
public:
    virtual std::vector<rid_t> where(const String& col_name, pred_t pred, rid_t lim = std::numeric_limits<rid_t>::max()) = 0;
    virtual TmpTable select(std::vector<String> names, const std::vector<rid_t>& rids) = 0;
};

class SavedTable;

class TmpTable : public Table {
private:
    std::vector<rec_t> recs;
public:
    // brute force
    std::vector<rid_t> where(const String& col_name, pred_t pred, rid_t lim) {
        std::vector<rid_t> ret;
        auto it = find_col(col_name);
        int col_id = it - cols.begin();
        for (unsigned i = 0; i < recs.size() && ret.size() < lim; i++) {
            if (pred.test(recs[i][col_id], it->get_datatype())) ret.push_back(i);
        }
        return ret;        
    }
    TmpTable select(std::vector<String> names, const std::vector<rid_t>& rids) {
        TmpTable ret;
        ret.recs.resize(rids.size());
        for (auto name: names) {
            auto col_id = find_col(name) - cols.begin();
            ret.cols.push_back(cols[col_id]);
            for (auto rid: rids) {
                ret.recs[rid].push_back(recs[rid][col_id]);
            }
        }
        return ret;        
    }

    static TmpTable from_strings(const String& title, const std::vector<String>& strs) {
        TmpTable ret;
        ret.cols.push_back(Column(title, ORANGE_VARCHAR));
        ret.recs.reserve(strs.size());
        for (auto &str: strs) {
            ret.recs.push_back({to_bytes(str)});
        }
        return ret;
    }

    friend class SavedTable;
    friend std::ostream& operator << (std::ostream& os, const TmpTable& table);
};

static String to_string(const byte_arr_t &bytes, datatype_t type) {
    if (bytes.front() == DATA_NULL) return "null";
    switch (type) {
        case ORANGE_VARCHAR: return String(bytes.data() + 1, bytes.data() + bytes.size());
        case ORANGE_CHAR: {
            int len = bytes.back() ? MAX_CHAR_LEN : strlen((char*)bytes.data() + 1);
            return String(bytes.data() + 1, bytes.data() + len + 1);
        }
        case ORANGE_INT: return std::to_string(*(int*)(bytes.data() + 1));
        case ORANGE_NUMERIC:
        case ORANGE_DATE: UNIMPLEMENTED
    }
    return "unimplemented";
}

constexpr static int width = 15 + 3;

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