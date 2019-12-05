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
    // 索引所在路径
    String path;
    std::vector<Column> cols;

    int key_size;
    BTree tree;

    Index(SavedTable& table, const String& path, int key_size, const std::vector<Column>& cols) : 
    table(table), path(path), cols(cols), key_size(key_size), tree(*this, key_size, path) {}
public:
    const std::vector<Column>& get_cols() const { return cols; }

    static Index* create(SavedTable& table, const std::vector<Column>& cols, const String& name);
    static Index* load(SavedTable& table, const std::vector<Column>& cols, const String& name);
    
    void load() {
        tree.load();
    }

    // raw: 索引值；val：真实值
    void insert(const byte_arr_t& raw, rid_t rid, const byte_arr_t& val) {
        tree.insert(raw.data(), rid, val);
        // f_data->seek_pos(rid * size)->write_bytes(raw.data(), size);
    }

    // 调用合适应该不会有问题8
    void remove(const byte_arr_t &raw, rid_t rid) {
        tree.remove(raw.data(), rid);
        // f_data->seek_pos(rid * size)->write<byte_t>(DATA_INVALID);
    }

    void update(const byte_arr_t &raw, rid_t rid, const byte_arr_t& val) {
        remove(raw, rid);
        insert(raw, rid, val);
    }

    // std::vector<rid_t> get_rids_value(const Orange::parser::op &op, const Orange::parser::data_value &value, rid_t lim) const {
    //     if (value.is_null()) return {};
    //     if (on) {
    //         return tree->query(op, value, lim);
    //     } else {
    //         std::vector<rid_t> ret;
    //         auto bytes = new byte_t[size];
    //         for (auto i: get_all()) {
    //             if (ret.size() >= lim) break;
    //             f_data->seek_pos(i * size)->read_bytes(bytes, size);
    //             if (Orange::cmp(restore(bytes), kind, op, value)) {
    //                 ret.push_back(i);
    //             }
    //         }            
    //         delete[] bytes;
    //         return ret;
    //     }
    // }

    // auto get_rids_null(bool not_null, rid_t lim) const {
    //     auto bytes = new byte_t[size];
    //     std::vector<rid_t> ret;
    //     for (auto i: get_all()) {
    //         if (ret.size() >= lim) break;
    //         f_data->seek_pos(i * size)->read_bytes(bytes, size);
    //         if (not_null && *bytes != DATA_NULL || !not_null && *bytes == DATA_NULL) {
    //             ret.push_back(i);
    //         }
    //     }
    //     delete[] bytes;
    //     return ret;
    // }

    friend class BTree;
};
