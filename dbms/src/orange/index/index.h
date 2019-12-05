#pragma once

#include <vector>

#include <defs.h>
#include <fs/file/file.h>
#include <fs/allocator/allocator.h>
#include <orange/index/b_tree.h>
#include "orange/parser/sql_ast.h"
#include "orange/common.h"

class SavedTable;

// 同时维护数据和索引，有暴力模式和数据结构模式
// 希望设计的时候索引模块不需要关注完整性约束，而交给其它模块
class Index {
private:
    SavedTable &table;
    String name;
    std::vector<Column> cols;
    bool primary, unique;

    int key_size;
    BTree *tree;

    Index(SavedTable& table, const String& name) : table(table), name(name) {}
    ~Index() {
        write_info();
        delete tree;
    }

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

public:
    String get_name() const { return name; }
    const std::vector<Column>& get_cols() const { return cols; }
    bool is_primary() const { return primary; }

    static Index* create(SavedTable& table, const String& name, const std::vector<Column>& cols, bool primary, bool unique);
    static Index* load(SavedTable& table, const String& name);
    static void drop(const Index* index) {
        auto path = index->get_path();
        delete index;
        fs::remove_all(path);
    }

    // raw: 索引值；val：真实值
    void insert(const byte_arr_t& raw, rid_t rid, const std::vector<byte_arr_t>& val) {
        tree->insert(raw.data(), rid, val);
    }

    // 调用合适应该不会有问题8
    void remove(const byte_arr_t &raw, rid_t rid) {
        tree->remove(raw.data(), rid);
    }

    void update(const byte_arr_t &raw, rid_t rid, const std::vector<byte_arr_t>& val) {
        remove(raw, rid);
        insert(raw, rid, val);
    }

    friend class BTree;
};
