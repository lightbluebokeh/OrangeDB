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
    
    BTree tree;

    // // 返回所在表的所有正在使用的 rid
    // std::vector<rid_t> get_all() const;

public:
    const std::vector<Column>& get_cols() const { return cols; }
    Index(SavedTable& table, const String& name, const std::vector<Column>& cols);

    void drop() {
        ORANGE_UNIMPL
    }

    // Index(SavedTable& table, orange_t kind, size_t size, const String& prefix, bool on) : table(table), kind(kind), size(size), prefix(prefix), on(on) {
    //     if (!fs::exists(data_name())) File::create(data_name());
    //     f_data = File::open(data_name());
    //     if (kind == orange_t::Varchar) allocator = new FileAllocator(vchar_name());
    // }
    // // 拒绝移动构造
    // Index(Index&& index) : table(index.table) { ORANGE_UNREACHABLE }
    // ~Index() {
    //     if (on) delete tree;
    //     if (f_data) f_data->close();
    //     delete allocator;
    // }

    void load() {
        tree.load();
    }

    // void turn_on() {
    //     if (!on) {
    //         on = 1;
    //         tree = new BTree(this, size, prefix);
    //         tree->init(f_data);
    //     }
    // }

    // void turn_off() {
    //     if (on) {
    //         on = 0;
    //         delete tree;
    //     }
    // }

    // raw: 索引值；val：真实值
    void insert(const byte_arr_t& raw, rid_t rid, const byte_arr_t& val) {
        // auto raw = store(val);
        tree.insert(raw.data(), rid, val);
        // f_data->seek_pos(rid * size)->write_bytes(raw.data(), size);
    }

    // 调用合适应该不会有问题8
    void remove(const byte_arr_t &raw, rid_t rid) {
        // auto raw = get_raw(rid);
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
