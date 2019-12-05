#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>

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

// 数据库中的表
class SavedTable : public Table {
private:
    // 各列数据接口
    struct ColumnDataHelper {
        File *f_data;
        int size;   // 每个值的大小
        // orange_t kind;
        ast::data_type type;
        FileAllocator *alloc;

        // root 为数据文件夹目录
        ColumnDataHelper(File* f_data, const Column& col, const String &root) : f_data(f_data), size(col.get_key_size()), type(col.get_datatype()) {
            if (type.kind == orange_t::Varchar) {
                alloc = new FileAllocator(root + col.get_name());
            } else {
                alloc = nullptr;
            }
        }
        ~ColumnDataHelper() {
            f_data->close();
            delete alloc;
        }

        // 对于 vchar 返回指针，其它直接返回真实值
        auto store(const byte_arr_t& key) const {
            switch (type.kind) {
                case orange_t::Varchar: return alloc->allocate_byte_arr(key);
                default: return key;
            }
        }
        auto restore(const_bytes_t k_raw) const {
            switch (type.kind) {
                case orange_t::Varchar: return alloc->read_byte_arr(*(size_t*)(k_raw + 1));
                default: return byte_arr_t(k_raw, k_raw + size);
            }
        }

        // 对于 varchar 返回指针
        auto get_raw(rid_t rid) const {
            auto bytes = new byte_t[size];
            f_data->seek_pos(rid * size)->read_bytes(bytes, size);
            auto ret = byte_arr_t(bytes, bytes + size);
            delete[] bytes;
            return ret;
        }
        //　返回 rid　记录此列的真实值
        auto get_val(rid_t rid) const {
            auto bytes = new byte_t[size];
            f_data->seek_pos(rid * size)->read_bytes(bytes, size);
            auto ret = restore(bytes);
            delete[] bytes;
            return ret;
        }
        // 返回一系列 rid 对应的该列值
        auto get_vals(std::vector<rid_t> rids) const {
            auto bytes = new byte_t[size];
            std::vector<byte_arr_t> ret;
            for (auto rid: rids) {
                f_data->seek_pos(rid * size)->read_bytes(bytes, size);
                ret.push_back(restore(bytes));
            }
            return ret;
        }

        void insert(rid_t rid, const ast::data_value& value) {
            auto raw = store(Orange::value_to_bytes(value, type));
            f_data->seek_pos(rid * size)->write_bytes(raw.data(), size);
        }

        auto filt_null(const std::vector<rid_t>& rids, bool not_null) const {
            auto bytes = new byte_t[size];
            std::vector<rid_t> ret;
            for (auto i: rids) {
                f_data->seek_pos(i * size)->read_bytes(bytes, size);
                if (not_null && *bytes != DATA_NULL || !not_null && *bytes == DATA_NULL) {
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
                    f_data->seek_pos(i * size)->read_bytes(bytes, size);
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

    SavedTable(int id, const String& name) : id(id), name(name), rid_pool(pool_name()) {}
    ~SavedTable() {
        for (auto data: col_data) delete data;
    }

    // info(还包括 Table::cols)
    rid_t rec_cnt;
    std::vector<f_key_t> f_keys;

    // map index by their name
    std::unordered_map<String, Index*> indexes;

    String root() { return get_root(name); }
    String info_name() { return root() + "info"; }
    String pool_name() { return root() + "rid.pl"; }
    String data_root() { return root() + "data/"; }
    String col_prefix(const String& col_name) { return data_root() + col_name; }
    String data_name(const String& col_name) { return col_prefix(col_name) + ".data"; };
    String index_root() { return root() + "index/"; }


    static constexpr char index_prefix[] = "index-";
    void write_info() {
        std::ofstream ofs(info_name());
        ofs << cols << ' ' << rec_cnt << ' ' << f_keys << ' ' << indexes.size();
        for (auto &[idx_name, idx]: indexes) {
            ofs << ' ' << (index_prefix + idx_name);
        }
        ofs << std::endl;
    }
    void read_info() {
        std::ifstream ifs(info_name());
        ifs >> cols >> rec_cnt >> f_keys;
        for (auto &col: cols) {
            col_data.push_back(new ColumnDataHelper(File::open(col.get_name()), col, data_root()));
        }
        int index_num;
        ifs >> index_num;
        while (index_num--) {
            String idx_name;
            ifs >> idx_name;
            idx_name = idx_name.substr(sizeof(index_prefix) - 1);   // 截断
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

    static void check_db() { orange_ensure(Orange::using_db(), "use some database first"); }
    
    void on_create(const std::vector<Column>& cols, const std::vector<String>& p_key, const std::vector<f_key_t>& f_keys) {
        this->cols = cols;
        this->f_keys = f_keys;
        this->rec_cnt = 0;
        File::create(info_name());
        write_info();
        rid_pool.init();
        fs::create_directory(data_root());
        for (auto &col: cols) {
            col_data.push_back(new ColumnDataHelper(File::create_open(col.get_name()), col, data_root()));
        }
        fs::create_directory(index_root());
        if (p_key.size()) add_p_key("", p_key);

        ORANGE_UNIMPL
    }

    // 表的文件夹名
    static String get_root(const String& name) { return name + "/"; }

    // 在打开的表里面找
    static SavedTable* get_opened(const String& name) {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (tables[i] && tables[i]->name == name) {
                return tables[i];
            }
        }
        return nullptr;
    }

    // 获取所有 rid
    std::vector<rid_t> all() const override { return rid_pool.all(); }

    std::pair<bool, std::vector<rid_t>> filt_index(const std::vector<rid_t>& rids, const ast::op& op, const ast::data_value& value) const {
        return {0, {}};
    }
    std::vector<rid_t> filt(const std::vector<rid_t>& rids, const ast::single_where& where) const override {
        if (where.is_null_check()) {    // is [not] null 直接暴力，反正对半
            auto &null = where.null_check();
            return col_data[get_col_id(null.col_name)]->filt_null(rids, !null.is_not_null);
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
                    orange_ensure(table_name == name, "unknown table name in selector: `" + table_name);
                }
                auto &col1 = get_col(where_op.col_name), &col2 = get_col(col.col_name);
                auto kind1 = col1.get_datatype_kind(), kind2 = col2.get_datatype_kind();
                auto data1 = col_data[col1.get_id()], data2 = col_data[col2.get_id()];
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
        return {0, {}};
    }
    std::vector<rid_t> where(const ast::where_clause &where, rid_t lim = MAX_RID) const override {
        if (op_null(where)) return {};
        auto [ok, ret] = where_index(where, lim);
        return ok ? ret : Table::where(where, lim);
    }

    byte_arr_t get_field(int col_id, rid_t rid) const override { return col_data[col_id]->get_val(rid); }

    void check_insert(const ast::data_values& values) const {
        ORANGE_UNIMPL
    }
    void check_delete(rid_t rid) const {
        ORANGE_UNIMPL
    }
    // 获取一列所有默认值
    auto get_dft_vals() const {
        ast::data_values ret;
        for (auto &col: cols) {
            ret.push_back(col.get_dft());
        }
        return ret;
    }

    auto get_raws(const std::vector<Column>& cols, rid_t rid) const {
        byte_arr_t ret;
        for (auto &col: cols) {
            auto tmp = col_data[col.get_id()]->get_raw(rid);
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
        for (auto &f_key: f_keys) {
            if (f_key.name == name) {
                return indexes.count(name) ? indexes[name] : nullptr;
            }
        }
        return nullptr;
    }
    const Index* get_f_key(const String& name) const {
        for (auto &f_key: f_keys) {
            if (f_key.name == name) {
                return indexes.count(name) ? indexes.at(name) : nullptr;
            }
        }
        return nullptr;
    }
    auto get_f_keys() {
        std::vector<Index*> ret;
        for (auto f_key: f_keys) {
            ret.push_back(indexes.at(f_key.name));
        }
        return ret;
    }
    Index* get_index(const String& name) {
        if (indexes.count(name)) {
            auto ret = indexes[name];
            return ret != get_p_key() && ret != get_f_key(name) ? ret : nullptr;
        }
        return nullptr;
    }
    const Index* get_index(const String& name) const {
        if (indexes.count(name)) {
            auto ret = indexes.at(name);
            return ret != get_p_key() && ret != get_f_key(name) ? ret : nullptr;
        }
        return nullptr;
    }
public:
    static bool create(const String& name, const std::vector<Column>& cols, const std::vector<String>& p_key,
                       const std::vector<f_key_t>& f_keys) {
        check_db();
        orange_ensure(!fs::exists(get_root(name)), "table `" + name + "` exists");
        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        try {
            table->on_create(cols, p_key, f_keys);
        } catch (OrangeException& e) {
            drop(name);
            throw e;
        }
        return 1;
    }

    static SavedTable* get(const String& name) {
        check_db();
        orange_ensure(fs::exists(get_root(name)), "table `" + name + "` does not exists");
        auto table = get_opened(name);
        if (!table) {
            table = new_table(name);
            table->read_info();
            table->rid_pool.load();
        }
        return table;
    }

    TmpTable description() {
        TmpTable ret;
        ret.cols.push_back(Column("Name"));
        ret.cols.push_back(Column("null"));
        ret.cols.push_back(Column("type"));
        ret.recs.resize(cols.size());
        for (unsigned i = 0; i < cols.size(); i++) {
            ret.recs[i].push_back(Orange::to_bytes(cols[i].get_name()));
            ret.recs[i].push_back(Orange::to_bytes(cols[i].is_nullable() ? "nullable" : "not null"));
            ret.recs[i].push_back(Orange::to_bytes(cols[i].type_string()));
        }
        return ret;
    }

    // 关闭表，写回缓存，数据在 index 的析构中写回
    bool close() {
        orange_ensure(this == tables[id], "this is magic");
        write_info();
        free_table(this);
        return 1;
    }

    static bool drop(const String& name) {
        auto table = get_opened(name);
        if (table) table->close();
        return fs::remove_all(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (auto &table : tables) {
            if (table) {
                orange_ensure(table->close(), "close table failed");
                delete table;
                table = nullptr;
            }
        }
    }

    // 按列插入，会先尝试把输入的值补全
    void insert(const std::vector<std::pair<String, ast::data_value>>& values) {
        std::unordered_map<String, ast::data_value> map;
        for (auto [name, val]: values) {
            orange_ensure(has_col(name), "insertion failed: unknown column name");
            map[name] = val;
        }
        std::vector<ast::data_value> values_all;
        for (auto &col: cols) {
            auto it = map.find(col.get_name());
            if (it == map.end()) {
                values_all.push_back(col.get_dft());
            } else {
                values_all.push_back(it->second);
            }
        }
        insert(values_all);
    }

    // 直接插入完整的一条
    void insert(std::vector<ast::data_value> values) {
        // 检查参数个数是否等于列数
        orange_ensure(values.size() == cols.size(), "insertion failed: expected " + std::to_string(cols.size()) + " values while " + std::to_string(values.size()) + "given");
        rec_t rec;
        for (unsigned i = 0; i < cols.size(); i++) {

        }
        // for (auto &value: values) rec.push_back(Orange::value_to_bytes(value, col_size));
        // insert_internal(values);
    }

    void insert(const std::vector<String>& col_names, ast::data_values_list values_list) {
        for (auto &values: values_list) {
            orange_ensure(col_names.size() == values.size(), "expected " + std::to_string(col_names.size()) + " values, while " + std::to_string(values.size()) + " given");
        }
        std::vector<int> col_ids;
        for (auto col_name: col_names) col_ids.push_back(get_col_id(col_name));
        auto insert_vals = get_dft_vals();
        for (auto &values: values_list) {
            for (int i = 0; i < col_ids.size(); i++) {
                auto col_id = col_ids[i];
                insert_vals[col_id] = values[i];    // 如果有重复列，后面的会覆盖前面的
            }
            values = insert_vals;
        }
        insert(values_list);
    }
    void insert(const ast::data_values_list& values_list) {
        for (auto &values: values_list) {
            orange_ensure(cols.size() == values.size(), "expected " + std::to_string(cols.size()) + " values, while " + std::to_string(values.size()) + " given");
            check_insert(values);
        }
        for (auto &values: values_list) {
            for (int i = 0; i < cols.size(); i++) {
                col_data[i]->insert(rid_pool.new_id(), values[i]);
            }
        }
        for (auto &[name, index]: indexes) {

        }
    }
    void delete_where(const ast::where_clause& where) {
        auto rids = this->where(where);
        for (auto rid: rids) check_delete(rid);
    }
    // TmpTable select_where(std::vector<String>& names, ast::where_clause& where) const 

    TmpTable select(const std::vector<String>& names, const ast::where_clause& where) const override {
        TmpTable ret;
        std::vector<int> col_ids = get_col_ids(names);
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

    // void update(const String& col_name, std::function<byte_arr_t(byte_arr_t)> expr, const std::vector<rid_t>& rids) {
    //     int col_id = get_col_id(col_name);
    //     auto &idx = indices[col_id];
    //     auto &col = cols[col_id];
    //     for (auto rid: rids) {
    //         auto new_val = expr(idx.get_val(rid));
    //         orange_ensure(col.test(new_val), "update failed: new value fails constraint");
    //         idx.update(new_val, rid);
    //     }
    // }

    void create_index(const String& idx_name, const std::vector<String>& col_names, bool primary, bool unique) {
        if (!primary && idx_name == PRIMARY_KEY_NAME) throw OrangeException("this name is reserved for primary key");
        orange_assert(!idx_name.empty(), "index name cannot be empty");
        if (primary) orange_assert(unique, "primary key must be unique");
        orange_ensure(!indexes.count(idx_name), "already has index named `" + idx_name + "`");
        auto idx_cols = get_cols(col_names);
        auto index = Index::create(*this, idx_name, idx_cols, primary, unique);
        indexes[idx_name] = index;
    }
    void drop_index(const String& idx_name) {
        auto index = get_index(idx_name);
        orange_ensure(index != nullptr, "index `" + name + "` does not exists");
        Index::drop(index);
        indexes.erase(idx_name);
    }
    void add_p_key(String name, const std::vector<String>& col_names) {
        orange_ensure(get_p_key() == nullptr, "already has primary key");
        if (name.empty()) name = PRIMARY_KEY_NAME;
        create_index(name, col_names, true, true);
    }
    void drop_p_key(const String& name) {
        auto p_key = get_p_key();
        orange_ensure(p_key && p_key->get_name() == name, "primary key named `" + name + "` does not exist");
        // TODO: check other tables' foreign key
        ORANGE_UNIMPL
        Index::drop(p_key);
    }
    void add_f_key(const f_key_t& f_key) {
        auto ref_tbl = SavedTable::get(f_key.ref_tbl);
        auto ref_p_key = ref_tbl->get_p_key();
        // 没有映射到那张表的主键
        orange_ensure(![&] {
            if (!ref_p_key || ref_p_key->get_cols().size() != f_key.ref_list.size()) return 0;
            for (unsigned i = 0; i < f_key.ref_list.size(); i++) {
                if (ref_p_key->get_cols()[i].get_name() != f_key.ref_list[i]) return 0;
            }
            return 1;
        }(), "should map to the primary key of table `" + f_key.ref_tbl + "`");
        create_index(f_key.name, f_key.list, false, false);
    }
    void drop_f_key(const String& name) {
        auto f_key = get_f_key(name);
        orange_ensure(f_key, "foreign key named `" + name + "` does not exist");
        Index::drop(f_key);
    }

    friend class Index;
};
