#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "defs.h"
#include "fs/file/file.h"
#include "orange/index/index.h"
#include "orange/orange.h"
#include "orange/table/column.h"
#include "orange/table/key.h"
#include "orange/table/table_base.h"
#include "utils/id_pool.h"
#include "orange/parser/sql_ast.h"
#include "fs/allocator/allocator.h"
#include "exceptions.h"

// 数据库中的表
class SavedTable : public Table {
private:
    // 各列数据接口
    struct ColumnDataHelper {
        File *f_data;
        int size;   // 每个值的大小
        ast::data_type type;
        FileAllocator *alloc;
        String root, name;

        // root 为数据文件夹目录
        ColumnDataHelper(File* f_data, const Column& col, const String &root, const String& name) : f_data(f_data), size(col.get_key_size()), type(col.get_datatype()), root(root), name(name) {
            if (type.kind == orange_t::Varchar) {
                alloc = new FileAllocator(root + col.get_name() + ".v");
            } else {
                alloc = nullptr;
            }
        }
        ~ColumnDataHelper() {
            f_data->close();
            delete alloc;
        }

        // 对于 varchar 类型，得到最长的长度
        int max_len(const std::vector<rid_t>& rids) {
            int ret = 0;
            for (auto rid: rids) {
                auto val = get_val(rid);
                ret = std::max(ret, int(strlen(String(val.begin() + 1, val.end()).data())));
            }
            return ret;
        }

        void change(const ast::field_def& def, const std::vector<rid_t>& all) {
            if (def.col_name != name) {
                f_data->close();
                fs::rename(root + name, root + def.col_name);
                name = def.col_name;
                f_data = File::open(root + name);
            }
            auto new_type = def.type;
            orange_check(type.is_string() && new_type.is_string(), Exception::change_nonstring());
            if (type.kind == orange_t::Char) {
                orange_check(type.int_value() <= new_type.int_value(), Exception::shrink_char());
                type = new_type;
                size = type.key_size();
                auto vals = get_vals(all);
                if (new_type.kind == orange_t::Char) {
                    for (unsigned i = 0; i < all.size(); i++) {
                        auto rid = all[i];
                        auto val = vals[i];
                        // 补零
                        val.resize(size);
                        // 不用转为 raw
                        f_data->write_bytes(rid * size, val.data(), size);
                    }
                } else {
                    alloc = new FileAllocator(root + def.col_name + ".v");
                    for (unsigned i = 0; i < all.size(); i++) {
                        auto rid = all[i];
                        auto val = vals[i];
                        auto raw = store(val);
                        f_data->write_bytes(rid * size, raw.data(), size);
                    }
                }
            } else {
                if (type.int_value() > new_type.int_value()) {
                    orange_check(max_len(all) <= new_type.int_value(), Exception::short_varchar());
                }
                type = new_type;
                size = type.key_size();
                if (new_type.kind == orange_t::Char) {
                    auto vals = get_vals(all);
                    for (unsigned i = 0; i < all.size(); i++) {
                        vals[i].resize(size);
                        insert(all[i], vals[i]);
                    }
                } else {
                    // varchar 长度限制改变，啥也不用做
                }
            }
        }

        // 对于 vchar 返回指针，其它直接返回真实值
        byte_arr_t store(const byte_arr_t& key) const {
            switch (type.kind) {
                case orange_t::Varchar: return alloc->allocate_byte_arr(key);
                default: return key;
            }
        }
        byte_arr_t restore(const_bytes_t k_raw) const {
            switch (type.kind) {
                case orange_t::Varchar: return alloc->read_byte_arr(*(size_t*)(k_raw + 1));
                default: return byte_arr_t(k_raw, k_raw + size);
            }
        }

        // 对于 varchar 返回指针
        auto get_raw(rid_t rid) const {
            auto bytes = new byte_t[size];
            f_data->read_bytes(rid * size, bytes, size);
            auto ret = byte_arr_t(bytes, bytes + size);
            delete[] bytes;
            return ret;
        }
        //　返回 rid　记录此列的真实值
        byte_arr_t get_val(rid_t rid) const {
            auto bytes = new byte_t[size];
            f_data->read_bytes(rid * size, bytes, size);
            auto ret = restore(bytes);
            delete[] bytes;
            return ret;
        }
        // 返回一系列 rid 对应的该列值
        std::vector<byte_arr_t> get_vals(const std::vector<rid_t>& rids) const {
            auto bytes = new byte_t[size];
            std::vector<byte_arr_t> ret;
            for (auto rid : rids) {
                f_data->read_bytes(rid * size, bytes, size);
                ret.push_back(restore(bytes));
            }
            return ret;
        }

        void insert(rid_t rid, const ast::data_value& value) {
            auto raw = store(Orange::value_to_bytes(value, type));
            f_data->write_bytes(rid * size, raw.data(), size);
        }
        void insert(rid_t rid, const byte_arr_t& bytes) {
            auto raw = store(bytes);
            f_data->write_bytes(rid * size, raw.data(), size);
        }

        void remove(rid_t rid) {
            if (type.kind == orange_t::Varchar) {
                alloc->free(f_data->read<size_t>(rid * size + 1));  // Varchar size=9，后 8 位为 offset
            }
            f_data->write<byte_t>(rid * size, DATA_INVALID);
        }

        auto filt_null(const std::vector<rid_t>& rids, bool not_null) const {
            auto bytes = new byte_t[size];
            std::vector<rid_t> ret;
            for (auto i: rids) {
                f_data->read_bytes(i * size, bytes, size);
                if ((not_null && *bytes != DATA_NULL) || (!not_null && *bytes == DATA_NULL)) {
                    ret.push_back(i);
                }
            }
            delete[] bytes;
            return ret;
        }
        auto filt_value(const std::vector<rid_t>& rids, const ast::op op, const ast::data_value& value) const {
            std::vector<rid_t> ret;
            if (!value.is_null()) { // null 直接返回空
                auto bytes = new byte_t[size];
                for (auto i: rids) {
                    f_data->read_bytes(i * size, bytes, size);
                    if (Orange::cmp(restore(bytes), type.kind, op, value)) {
                        ret.push_back(i);
                    }
                }
                delete[] bytes;
            }
            return ret;
        }
    };

    std::vector<ColumnDataHelper*> col_data;

    int id;
    String name;
    IdPool<rid_t> rid_pool;

    SavedTable(int id, String name) : id(id), name(std::move(name)), rid_pool(pool_name()) {}
    ~SavedTable() override {
        for (auto data : col_data) delete data;
        for (auto& [name, index] : indexes) delete index;
    }

    // info(还包括 Table::cols)
    rid_t rec_cnt;
    // std::vector<f_key_t> f_key_defs;
    std::map<String, f_key_t> f_key_defs;
    // 索引到该表主键的所有外键：外键名，源表+外键定义
    std::map<String, std::pair<String, f_key_t>> f_key_rev;

    // map index by their name
    std::unordered_map<String, Index*> indexes;

    String root() { return get_root(name); }
    String info_name() { return root() + "info"; }
    String pool_name() { return root() + "rid"; }
    String data_root() { return root() + "data/"; }
    String col_prefix(const String& col_name) { return data_root() + col_name; }
    String data_name(const String& col_name) { return col_prefix(col_name); };
    String index_root() { return root() + "index/"; }


    // static constexpr char index_prefix[] = "index-";
    void write_info() {
        std::ofstream ofs(info_name());
        ofs << cols << ' ' << rec_cnt << ' ' << f_key_defs.size();
        for (auto &[name, f_key_def]: f_key_defs) {
            ofs << ' ' << f_key_def;
        }
        ofs << ' ' << indexes.size();
        for (auto &[idx_name, idx]: indexes) {
            ofs << ' ' << idx_name;
        }
        ofs << std::endl;
    }
    void read_info() {
        std::ifstream ifs(info_name());
        ifs >> cols >> rec_cnt;
        size_t f_key_size;
        ifs >> f_key_size;
        for (size_t i = 0; i < f_key_size; i++) {
            f_key_t f_key_def;
            ifs >> f_key_def;
            f_key_defs[f_key_def.name] = f_key_def;
        }
        for (auto &col: cols) {
            col_data.push_back(new ColumnDataHelper(File::open(data_name(col.get_name())), col, data_root(), name));
        }
        int index_num;
        ifs >> index_num;
        while (index_num--) {
            String idx_name;
            ifs >> idx_name;
            auto index = Index::load(*this, idx_name);
            indexes[idx_name] = index;
        }
    }

    static SavedTable* tables[MAX_TBL_NUM];
    static SavedTable* new_table(const String& name) {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (!tables[i]) {
                tables[i] = new SavedTable(i, name);
                tables[i]->id = i;
                return tables[i];
            }
        }
        throw OrangeException("no more tables plz >_<");
    }
    static void free_table(SavedTable* table) {
        if (table == nullptr) return;
        tables[table->id] = nullptr;
        delete table;
    }
    static void check_exists(const String& tbl_name) {
        orange_check(fs::exists(get_root(tbl_name)), "table `" + tbl_name + "` does not exists");
    }

    static void check_db() { orange_check(Orange::using_db(), Exception::no_database_used()); }

    void on_create(const std::vector<Column>& cols, const std::vector<String>& p_key_cols, const std::vector<f_key_t>& f_key_defs) {
        this->cols = cols;
        check_unique();
        this->rec_cnt = 0;
        File::create(info_name());
        write_info();
        rid_pool.init();
        fs::create_directory(data_root());
        for (auto &col: cols) {
            col_data.push_back(new ColumnDataHelper(File::create_open(data_name(col.get_name())), col, data_root(), name));
        }
        fs::create_directory(index_root());
        if (!p_key_cols.empty()) add_p_key("", p_key_cols);
        for (auto &f_key_def: f_key_defs) add_f_key(f_key_def);
    }

    // 表的文件夹名
    static String get_root(const String& name) { return name + "/"; }

    // 在打开的表里面找，找不到返回 null
    static SavedTable* get_opened(const String& name) {
        check_db();
        for (auto& table : tables) {
            if (table && table->name == name) {
                return table;
            }
        }
        return nullptr;
    }

    // 获取所有 rid
    std::vector<rid_t> all() const override { return rid_pool.all(); }

    // 尝试用单列索引
    // 懒了，慢就慢吧
    std::pair<bool, std::vector<rid_t>> filt_index(const std::vector<rid_t>& , const ast::op& , const ast::data_value& ) const {
        return {false, {}};
        // constexpr int threshold = 1000;
        // if (value.is_null()) return {1, {}};
        // // 太少了直接暴力就好
        // if (rids.size() <= threshold) return {0, {}};
        // auto index = get_index
        // return {0, {}};
    }
    std::vector<rid_t> filt(const std::vector<rid_t>& rids, const ast::single_where& where) const override {
        if (where.is_null_check()) {    // is [not] null 直接暴力，反正对半
            auto &null = where.null_check();
            return col_data[get_col_id(null.col_name)]->filt_null(rids, null.is_not_null);
        } else {
            auto &where_op = where.op();
            auto &expr = where_op.expression;
            auto &op = where_op.operator_;
            if (expr.is_value()) {
                auto &value = expr.value();
                if (value.is_null()) return {}; // null 直接返回空咯
                // 尝试用单列索引
                auto [ok, ret] = filt_index(rids, op, value);
                if (ok) return ret;
                return col_data[get_col_id(where_op.col_name)]->filt_value(rids, op, value);
            } else if (expr.is_column()) {
                auto &col = where_op.expression.col();
                if (col.table_name.has_value()) {
                    auto &table_name = col.table_name.get();
                    orange_check(table_name == name, "unknown table name in selector: `" + table_name);
                }
                auto &col1 = get_col(where_op.col_name), &col2 = get_col(col.col_name);
                auto kind1 = col1.get_datatype_kind(), kind2 = col2.get_datatype_kind();
                auto data1 = col_data[get_col_id(col1.get_name())], data2 = col_data[get_col_id(col2.get_name())];
                std::vector<rid_t> ret;
                for (auto rid: rids) {
                    if (Orange::cmp(data1->get_val(rid), kind1, where_op.operator_, data2->get_val(rid), kind2)) {
                        ret.push_back(rid);
                    }
                }
                return ret;
            } else {
                ORANGE_UNREACHABLE
            }
        }
    }

    // 尝试使用直接使用索引求解 where
    std::pair<bool, std::vector<rid_t>> where_index(const ast::where_clause &where, rid_t lim = MAX_RID) const {
        if (where.empty()) return {1, all()};
        std::vector<int> col_ids;
        std::vector<String> col_names;
        std::vector<std::pair<String, Orange::pred_t>> all_preds;
        for (auto &single_where: where) {
            // 使用索引时查询中不允许包含 null
            if (single_where.is_null_check()) return {0, {}};
            auto &op = single_where.op();
            auto &expr = op.expression;
            // Neq 也不做了，太烦了
            if (expr.is_column() || op.operator_ == ast::op::Neq) return {0, {}};
            auto &value = expr.value();
            if (value.is_null()) return {1, {}};
            int col_id = get_col_id(op.col_name);
            col_ids.push_back(col_id);
            col_names.push_back(op.col_name);
            // all_preds.push_back({op.operator_, value});
            all_preds.push_back(std::make_pair(op.col_name, Orange::pred_t{op.operator_, value}));
        }
        std::sort(col_names.begin(), col_names.end());
        col_names.resize(std::unique(col_names.begin(), col_names.end()) - col_names.begin());
        auto index = get_index(col_names);
        if (!index) return {0, {}};
        std::vector<Orange::preds_t> preds_list;
        preds_list.resize(index->get_cols().size());
        for (auto &[col_name, pred]: all_preds) {
            preds_list[index->get_col_rank(col_name)].push_back(pred);
        }
#ifdef DEBUG
        std::cerr << "query use index" << std::endl;
#endif
        return {1, index->query(preds_list, lim)};
    }

    // 单个表的 where
    // 重写的函数不允许有默认参数
    std::vector<rid_t> where(const ast::where_clause &where, rid_t lim = MAX_RID) const override {
        if (check_op_null(where)) return {};
        auto [ok, ret] = where_index(where, lim);
        return ok ? ret : Table::where(where, lim);
    }

    byte_arr_t get_field(int col_id, rid_t rid) const override { return col_data[col_id]->get_val(rid); }

    void check_insert(const ast::data_values& values) const {
        orange_check(cols.size() == values.size(), "expected " + std::to_string(cols.size()) + " values, while " + std::to_string(values.size()) + " given");
        // 列完整性约束
        for (unsigned i = 0; i < cols.size(); i++) {
            const auto &[ok, msg] = cols[i].check(values[i]);
            orange_check(ok, msg);
        }
        // 检查 unique 约束和外键约束
        for (auto &[name, index]: indexes) if (index->is_unique()) {
            bool has_null = 0, all_null = 1;
            std::vector<byte_arr_t> vals;
            for (auto &col: index->get_cols()) {
                if (values[get_col_id(col.get_name())].is_null()) {
                    has_null = 1;
                } else {
                    all_null = false;
                }
                vals.push_back(Orange::value_to_bytes(values[get_col_id(col.get_name())], col.get_datatype()));
            }
            // unique 无法约束 null
            if (!has_null) orange_check(!index->constains(vals), "fail unique constraint");
            // 检查外键约束
            if (f_key_defs.count(name)) {
                auto &f_key_def = f_key_defs.at(name);
                if (has_null) {
                    orange_check(all_null, "foreign key columns must either be null or non-null together");
                } else {
                    orange_check(SavedTable::get(f_key_def.ref_tbl)->get_p_key()->constains(vals), "foreign key map missed");
                }
            }
        }
    }
    // void check_delete(rid_t) const {
        // always deletable
        // auto p_key = get_p_key();
        // if (p_key) {
        //     auto vals = get_fields(p_key->get_cols(), rid);
        //     for (auto &[name, src_f_key]: f_key_rev) {
        //         auto &[src, f_key_def] = src_f_key;
        //         // 默认是 set null，如果有行映射过来，需要检查对应列是不是 not null
        //         auto f_key = SavedTable::get(src)->get_f_key(f_key_def.name);
        //         if (f_key->constains(vals)) {
        //             for (auto &col: f_key->get_cols()) {
        //                 orange_check(col.is_nullable(), "delete record that some non-null columns reference to it");
        //             }
        //         }
        //     }
        // }
    // }
    rid_t delete_record(rid_t rid) {
        if (rid_pool.contains(rid)) return 0;
        for (auto &[idx_name, index]: indexes) {
            index->remove(get_raws(index->get_cols(), rid), rid);
        }
        for (unsigned i = 0; i < cols.size(); i++) {
            col_data[i]->remove(rid);
        }
        rid_pool.free_id(rid);

        auto ret = 1;
        auto p_key = get_p_key();
        if (p_key) {
            // 级联删除
            auto raws = get_raws(p_key->get_cols(), rid);
            for (auto &[idx_name, src_f_key]: f_key_rev) {
                auto &[src_name, f_key_def] = src_f_key;
                auto src = SavedTable::get(src_name);
                auto f_key = src->get_f_key(f_key_def.name);
                for (auto ref_id: f_key->get_on_key(raws.data())) {
                    ret += src->delete_record(ref_id);
                }
            }
        }
        return ret;
    }

    // 获取一列所有默认值
    auto get_dft_vals() const {
        ast::data_values ret;
        for (auto &col: cols) {
            ret.push_back(col.get_dft());
        }
        return ret;
    }

    byte_arr_t get_raws(const std::vector<Column>& cols, rid_t rid) const {
        byte_arr_t ret;
        for (auto &col: cols) {
            auto tmp = col_data[get_col_id(col.get_name())]->get_raw(rid);
            ret.insert(ret.end(), tmp.begin(), tmp.end());
        }
        return ret;
    }

    Index* get_p_key() {
        for (auto &[name, index]: indexes) {
            if (index->is_primary()) return index;
        }
        return nullptr;
    }
    const Index* get_p_key() const {
        for (auto &[name, index]: indexes) {
            if (index->is_primary()) return index;
        }
        return nullptr;
    }
    Index* get_f_key(const String& name) {
        for (auto& [_, f_key] : f_key_defs) {
            if (f_key.name == name) {
                return indexes.count(name) ? indexes[name] : nullptr;
            }
        }
        return nullptr;
    }
    [[nodiscard]] const Index* get_f_key(const String& name) const {
        for (auto& [_, f_key] : f_key_defs) {
            if (f_key.name == name) {
                return indexes.count(name) ? indexes.at(name) : nullptr;
            }
        }
        return nullptr;
    }
    // 获取所有外键索引
    auto get_f_keys() {
        std::vector<Index*> ret;
        for (auto [_, f_key]: f_key_defs) {
            ret.push_back(indexes.at(f_key.name));
        }
        return ret;
    }
    // 通过索引名称获取索引
    Index* get_index(const String& idx_name) {
        if (indexes.count(idx_name)) {
            auto ret = indexes[idx_name];
            return ret != get_p_key() && ret != get_f_key(idx_name) ? ret : nullptr;
        }
        return nullptr;
    }
    const Index* get_index(const String& idx_name) const {
        if (indexes.count(idx_name)) {
            auto ret = indexes.at(idx_name);
            return ret != get_p_key() && ret != get_f_key(idx_name) ? ret : nullptr;
        }
        return nullptr;
    }
    // 获取首列相同，其余列包含的索引；尽可能选键大小小的
    Index* get_index(const std::vector<String>& col_names) {
        Index* ret = nullptr;
        for (auto &[idx_name, index]: indexes) {
            if (index->get_cols().front().get_name() == col_names.front()) {
                // 暴力判包含关系
                bool test = 1;
                for (unsigned i = 1; i < col_names.size(); i++) {
                    if (index->get_col_rank(col_names[i]) == -1) {
                        test = 0;
                        break;
                    }
                }
                if (test && (!ret || ret->get_key_size() > index->get_key_size())) ret = index;
            }
        }
        return ret;
    }
    const Index* get_index(const std::vector<String>& col_names) const {
        const Index* ret = nullptr;
        for (auto &[idx_name, index]: indexes) {
            if (index->get_cols().front().get_name() == col_names.front()) {
                // 暴力判包含关系
                bool test = 1;
                for (unsigned i = 1; i < col_names.size(); i++) {
                    if (index->get_col_rank(col_names[i]) == -1) {
                        test = 0;
                        break;
                    }
                }
                if (test && (!ret || ret->get_key_size() > index->get_key_size())) ret = index;
            }
        }
        return ret;
    }

    // 检查各列是不是不同
    void check_unique() const {
        std::unordered_set<String> set;
        for (auto &col: cols) {
            orange_check(!set.count(col.get_name()), "duplicate column name is not allowed");
            set.insert(col.get_name());
        }
    }
public:
    [[nodiscard]] String get_name() const override { return this->name; }

    static bool create(const String& name, const std::vector<Column>& cols, const std::vector<String>& p_key,
                       const std::vector<f_key_t>& f_key_defs) {
        check_db();
        orange_check(!fs::exists(get_root(name)), "table `" + name + "` exists");
        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        try {
            table->on_create(cols, p_key, f_key_defs);
        } catch (OrangeException& e) {
            drop(name);
            throw e;
        }
        return true;
    }

    static SavedTable* get(const String& name) {
        check_db();
        check_exists(name);
        auto table = get_opened(name);
        if (!table) {
            table = new_table(name);
            table->read_info();
            table->rid_pool.load();
        }
        return table;
    }

    [[nodiscard]] TmpTable description() const {
        TmpTable ret;
        ret.cols.emplace_back("Name");
        ret.cols.emplace_back("null");
        ret.cols.emplace_back("type");
        ret.recs.resize(cols.size());
        for (unsigned i = 0; i < cols.size(); i++) {
            ret.recs[i].push_back(Orange::to_bytes(cols[i].get_name()));
            ret.recs[i].push_back(
                Orange::to_bytes(cols[i].is_nullable() ? "nullable" : "not null"));
            ret.recs[i].push_back(Orange::to_bytes(cols[i].type_string()));
        }
        return ret;
    }

    // 关闭表，写回数据缓存
    bool close() {
        orange_assert(this == tables[id], "this is magic");
        write_info();
        free_table(this);
        return true;
    }

    static bool drop(const String& name) {
        check_exists(name);
        // 如果已经打开了需要先关闭
        auto table = get_opened(name);
        if (table) table->close();
        return fs::remove_all(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (auto &table : tables) {
            if (table) {
                orange_assert(table->close(), "close table failed");
                table = nullptr;
            }
        }
    }

    [[nodiscard]] bool has_index(const String& idx_name) const { return indexes.count(idx_name); }

    // 返回插入成功条数
    rid_t insert(const std::vector<String>& col_names, ast::data_values_list values_list) {
        for (auto &values: values_list) {
            orange_check(col_names.size() == values.size(), "expected " + std::to_string(col_names.size()) + " values, while " + std::to_string(values.size()) + " given");
        }
        std::vector<int> col_ids;
        for (const auto& col_name : col_names) col_ids.push_back(get_col_id(col_name));
        auto insert_vals = get_dft_vals();
        for (auto &values: values_list) {
            for (unsigned i = 0; i < col_ids.size(); i++) {
                auto col_id = col_ids[i];
                insert_vals[col_id] = values[i];    // 如果有重复列，后面的会覆盖前面的
            }
            values = insert_vals;
        }
        return insert(values_list);
    }
    rid_t insert(const ast::data_values_list& values_list) {
        for (auto &values: values_list) {
            check_insert(values);
        }

        std::vector<rid_t> new_ids;
        // 接下来已经可以确保可以插入
        for (auto &values: values_list) {
            auto new_id = rid_pool.new_id();
            new_ids.push_back(new_id);
            for (unsigned i = 0; i < cols.size(); i++) {
                col_data[i]->insert(new_id, values[i]);
            }
        }

        for (auto &[name, index]: indexes) {
            for (auto rid: new_ids) {
                index->insert(get_raws(index->get_cols(), rid), rid);
            }
        }
        return values_list.size();
    }

    // 返回被删除的数据条数
    rid_t delete_where(const ast::where_clause& where) {
        auto rids = this->where(where);
        // orange db 的删除不会有约束问题
        // for (auto rid: rids) check_delete(rid);
        rid_t ret = 0;
        for (auto rid: rids) ret += delete_record(rid);
        return ret;
    }

    TmpTable select(const std::vector<String>& col_names, const ast::where_clause& where) const override {
        TmpTable ret;
        std::vector<int> col_ids = get_col_ids(col_names);
        auto rids = this->where(where);
        ret.recs.resize(rids.size());
        for (auto col_id: col_ids) {
            ret.cols.push_back(cols[col_id]);
            // auto vals = indices[col_id].get_vals(rids);
            auto vals = col_data[col_id]->get_vals(rids);
            for (unsigned i = 0; i < rids.size(); i++) {
                ret.recs[i].push_back(vals[i]);
            }
        }
        return ret;
    }

    void update_where(const ast::set_clause& set, const ast::where_clause& where) {
        auto rids = this->where(where);
        std::map<String, ast::data_value> new_vals;
        for (auto &single_set: set) {
            new_vals[single_set.col_name] = single_set.val;
        }
        auto has_update = [&] (const std::vector<Column>& cols) {
            for (auto col: cols) if (new_vals.count(col.get_name())) return 1;
            return 0;
        };

        // 更新索引
        for (auto [_, index]: indexes) if (has_update(index->get_cols())) {
            std::vector<int> col_ids;
            for (auto col: index->get_cols()) col_ids.push_back(get_col_id(col.get_name()));
            for (auto rid: rids) {
                byte_arr_t new_raw;
                for (auto col: index->get_cols()) {
                    byte_arr_t tmp;
                    if (new_vals.count(col.get_name())) {
                        tmp = Orange::value_to_bytes(new_vals[col.get_name()], col.get_datatype());
                    } else {
                        tmp = col_data[get_col_id(col)]->get_raw(rid);
                    }
                    new_raw.insert(new_raw.end(), tmp.begin(), tmp.end());
                }
                index->update(get_raws(index->get_cols(), rid), new_raw, rid);
            }
        }

        // 更新数据
        for (auto &[col_name, val]: new_vals) {
            int col_id = get_col_id(col_name);
            for (auto rid: rids) {
                col_data[col_id]->insert(rid, val);
            }
        }
    }

    void create_index(const String& idx_name, const std::vector<String>& col_names, bool primary, bool unique) {
        if (!primary && idx_name == PRIMARY_KEY_NAME) throw OrangeException("this name is reserved for primary key");
        orange_assert(!idx_name.empty(), "index name cannot be empty");
        if (primary) orange_assert(unique, "primary key must be unique");
        orange_check(!has_index(idx_name), Exception::index_exists(idx_name, this->name));
        auto idx_cols = get_cols(col_names);
        auto index = Index::create(*this, idx_name, idx_cols, primary, unique);
        indexes[idx_name] = index;
    }
    void drop_index(const String& idx_name) {
        auto index = get_index(idx_name);
        orange_check(index != nullptr, Exception::index_not_exist(idx_name, this->name));
        Index::drop(index);
        indexes.erase(idx_name);
    }

    void add_p_key(String p_key_name, const std::vector<String>& col_names) {
        orange_check(get_p_key() == nullptr, "already has primary key");
        if (p_key_name.empty()) p_key_name = PRIMARY_KEY_NAME;
        create_index(p_key_name, col_names, true, true);
    }
    void drop_p_key(const String& p_key_name) {
        auto p_key = get_p_key();
        orange_check(p_key && p_key->get_name() == p_key_name, "primary key named `" + p_key_name + "` does not exist");
        orange_check(f_key_rev.empty(), Exception::drop_pk_fk_ref(name));
        Index::drop(p_key);
    }

    void add_f_key(const f_key_t& f_key_def) {
        auto ref_tbl = SavedTable::get(f_key_def.ref_tbl);
        auto ref_p_key = ref_tbl->get_p_key();
        // 没有映射到那张表的主键
        orange_check(![&] {
            if (!ref_p_key || ref_p_key->get_cols().size() != f_key_def.ref_list.size()) return 0;
            for (unsigned i = 0; i < f_key_def.ref_list.size(); i++) {
                if (ref_p_key->get_cols()[i].get_name() != f_key_def.ref_list[i]) return 0;
            }
            return 1;
        }(), "should map to the primary key of table `" + f_key_def.ref_tbl + "`");
        create_index(f_key_def.name, f_key_def.list, false, false);
        f_key_defs[f_key_def.name] = f_key_def;
        SavedTable::get(f_key_def.ref_tbl)->f_key_rev[f_key_def.name] = std::make_pair(this->name, f_key_def);
    }
    void drop_f_key(const String& f_key_name) {
        auto f_key = get_f_key(f_key_name);
        orange_check(f_key, "foreign key named `" + f_key_name + "` does not exist");
        Index::drop(f_key);
        auto &f_key_def = f_key_defs[f_key_name];
        f_key_defs.erase(f_key_name);
        SavedTable::get(f_key_def.ref_tbl)->f_key_rev.erase(f_key_name);
    }

    void add_col(const Column& new_col) {
        auto col_name = new_col.get_name();
        for (auto &col: cols) {
            orange_check(col_name != col.get_name(), Exception::col_exists(col_name, this->name));
        }
        cols.push_back(new_col);
        auto data = new ColumnDataHelper(File::create_open(data_name(col_name)), new_col, data_root(), name);
        col_data.push_back(data);
        // 新增一列时设置默认值，注意检查
        auto dft = new_col.get_dft();
        const auto &[ok, msg] = new_col.check(dft);
        orange_check(ok, msg);
        for (auto rid: all()) data->insert(rid, dft);
    }

    void drop_col(const String& col_name) {
        auto drop_col = get_col(col_name);
        // 如果有索引使用当前列，不允许删除
        for (auto &[idx_name, index]: indexes) {
            for (const auto& col : index->get_cols()) {
                orange_check(col.get_name() != drop_col.get_name(),
                             Exception::drop_index_col(col_name, idx_name, this->name));
            }
        }
        auto drop_id = get_col_id(drop_col.get_name());
        delete col_data[drop_id];
        // 暴力左移就可以了
        for (unsigned i = drop_id; i + 1 < col_data.size(); i++) {
            col_data[i] = col_data[i + 1];
            cols[i] = Column(cols[i + 1].get_name(), cols[i + 1].get_datatype(), cols[i + 1].is_nullable(), cols[i + 1].get_dft());
        }
        col_data.resize(col_data.size() - 1);
        cols.resize(cols.size() - 1);
    }

    void change_col(const String& col_name, const ast::field_def& def) {
        auto old_col = get_col(col_name);
        auto col_id = get_col_id(old_col.get_name());
        // 有索引使用的列不允许 change
        for (auto [idx_name, index]: indexes) {
            for (const auto& col : index->get_cols()) {
                orange_check(col.get_name() != col_name, Exception::change_index_col(col_name, idx_name, name));
            }
        }
        // 目前只允许在 char 和 varchar 之间转换，以及改变长度限制
        col_data[col_id]->change(def, all());
    }

    static void rename(const String& old_name, const String& new_name) {
        check_db();
        auto table = SavedTable::get_opened(old_name);  // 防止文件系统出 bug，先关掉表再重命名
        if (table) table->close();
        fs::rename(old_name, new_name);
    }

    friend class Index;
};
