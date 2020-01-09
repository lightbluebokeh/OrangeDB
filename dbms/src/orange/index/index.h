#pragma once

#include <utility>
#include <vector>

#include "defs.h"
#include "fs/file/file.h"
#include "fs/allocator/allocator.h"
#include "orange/index/b_tree.h"
#include "orange/parser/sql_ast.h"
#include "orange/common.h"

class SavedTable;
class Index {
private:
    SavedTable &table;
    String name;
    std::vector<Column> cols;
    bool primary, unique;

    int key_size;
    BTree *tree;

    Index(SavedTable& table, String name) : table(table), name(std::move(name)) {}

    // 获取第 i 个字段
    byte_arr_t restore(const_bytes_t k_raw, int i) const;
    // 获取全部字段
    std::vector<byte_arr_t> restore(const_bytes_t k_raw) const;
    String get_path() const;
    String info_name() const { return get_path() + "info"; }
    
    void write_info() {
        std::ofstream ofs(info_name());
        ofs << cols.size();
        for (auto &col: cols) {
            ofs << ' ' << col;
        }
        ofs << ' ' << primary << ' ' << unique << std::endl;
    }
    void read_info() {
        std::ifstream ifs(info_name());
        unsigned cols_num;
        ifs >> cols_num;
        cols.resize(cols_num);
        for (auto &col: cols) ifs >> col;
        ifs >> primary >> unique;
    }

    bool has_null(const byte_arr_t& raw) const {
        for (unsigned i = 0, sum; i < cols.size(); i++) {
            if (raw[sum] == DATA_NULL) return 1;
            sum += cols[i].get_key_size();
        }
        return 0;
    }
public:
    ~Index() {
        write_info();
        delete tree;
    }

    String get_name() const { return name; }
    int get_key_size() const { return key_size; }
    // 获取列在 index 中的排名；不存在返回 -1
    int get_col_rank(const String& col_name) const {
        for (unsigned i = 0; i < cols.size(); i++) {
            if (cols[i].get_name() == col_name) return i;
        }
        return -1;
    }
    std::vector<Column>& get_cols() { return cols; }
    const std::vector<Column>& get_cols() const { return cols; }
    bool is_primary() const { return primary; }
    bool is_unique() const { return unique; }
    bool constains(const std::vector<byte_arr_t>& vals) const {
        for (auto &val: vals) orange_assert(val.front() != DATA_NULL, "null is not supported");
        return tree->contains(vals);
    }
    // 返回所有等于对应 key 值的记录编号
    std::vector<rid_t> get_on_key(const_bytes_t raw, rid_t lim = MAX_RID) const {
        return tree->get_on_key(restore(raw), lim);
    }

    static Index* create(SavedTable& table, const String& name, const std::vector<Column>& cols, bool primary, bool unique);
    static Index* load(SavedTable& table, const String& name);
    static void drop(const Index* index) {
        auto path = index->get_path();
        delete index;
        fs::remove_all(path);
    }

    // raw: 索引值；val：真实值
    void insert(const byte_arr_t& raw, rid_t rid) {
        // 遇到有 null 的不必插
        if (has_null(raw)) return;
        tree->insert(raw.data(), rid);
    }

    // 调用合适应该不会有问题8
    void remove(const byte_arr_t &raw, rid_t rid) {
        if (has_null(raw)) return;
        tree->remove(raw.data(), rid);
    }

    std::vector<rid_t> query(const std::vector<Orange::preds_t>& preds_list, rid_t lim) const {
        return tree->query(preds_list, lim);
    }

    void update(const byte_arr_t &raw, const byte_arr_t& new_raw, rid_t rid) {
        remove(raw, rid);
        insert(new_raw, rid);
    }

    friend class BTree;
};
