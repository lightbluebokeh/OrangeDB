#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/index/index.h>
#include <orange/orange.h>
#include <orange/table/column.h>
#include <orange/table/key.h>
#include <orange/table/table_base.h>
#include <utils/id_pool.h>

// 数据库中的表
class SavedTable : public Table {
private:
    int id;
    String name;
    IdPool<rid_t> rid_pool;

    SavedTable(int id, const String& name) : id(id), name(name), rid_pool(pool_name()) {}

    // metadata
    rid_t rec_cnt;
    std::vector<String> p_key;
    std::vector<f_key_t> f_keys;

    std::vector<Index> indices;

    String root() { return get_root(name); }
    String data_root() { return root() + "data/"; }
    String col_prefix(const String& col_name) { return data_root() + col_name; }
    String metadata_name() { return root() + "metadata.tbl"; }
    String pool_name() { return root() + "rid.pl"; }

    void write_metadata() {
        // 现在看来确实有点智障，之后有时间再改吧
        File::open(metadata_name())->seek_pos(0)->write(cols, rec_cnt, p_key, f_keys)->close();
    }
    void read_metadata() {
        File::open(metadata_name())->seek_pos(0)->read(cols, rec_cnt, p_key, f_keys)->close();
        indices.reserve(cols.size());
        for (auto &col: cols) {
            indices.emplace_back(*this, col.get_datatype(), col.key_size(), col_prefix(col.get_name()), col.has_index());
            indices.back().load();
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

    // rec 万无一失
    void insert_internal(const std::vector<byte_arr_t>& rec) {
        rid_t rid = rid_pool.new_id();
        for (unsigned i = 0; i < rec.size(); i++) {
            indices[i].insert(rec[i], rid);
        }
        rec_cnt++;
    }

    void on_create(const std::vector<Column>& cols, const std::vector<String>& p_key, const std::vector<f_key_t>& f_keys) {
        this->cols = cols;
        this->p_key = p_key;
        this->f_keys = f_keys;
        this->rec_cnt = 0;
        File::create(metadata_name());
        write_metadata();
        rid_pool.init();
        fs::create_directory(data_root());
        indices.reserve(cols.size());   // 拒绝移动构造
        for (auto col: this->cols) {
            indices.emplace_back(*this, col.get_datatype(), col.key_size(), col_prefix(col.get_name()), col.has_index());
        }
    }

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
protected:
    std::vector<rid_t> all() const override { return rid_pool.all(); }

    byte_arr_t get_field(rid_t rid, int col_id) const override { return this->indices[col_id].get_val(rid); }
public:
    static bool create(const String& name, const std::vector<Column>& cols, const std::vector<String>& p_key,
                       const std::vector<f_key_t>& f_keys) {
        check_db();
        orange_ensure(!fs::exists(get_root(name)), "table `" + name + "` exists");
        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        table->on_create(cols, p_key, f_keys);
        return 1;
    }

    static SavedTable* get(const String& name) {
        check_db();
        orange_ensure(fs::exists(get_root(name)), "table `" + name + "` does not exists");
        auto table = get_opened(name);
        if (!table) {
            table = new_table(name);
            table->read_metadata();
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
            ret.recs[i].push_back(Orange::to_bytes(cols[i].nullable ? "nullable" : "not null"));
            ret.recs[i].push_back(Orange::to_bytes(cols[i].type_string()));
        }
        return ret;
    }

    // 关闭表，写回缓存，数据在 index 的析构中写回
    bool close() {
        orange_ensure(this == tables[id], "this is magic");
        write_metadata();
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
    void insert(const std::vector<std::pair<String, byte_arr_t>>& values) {
        std::unordered_map<String, byte_arr_t> map;
        for (auto [name, val]: values) {
            orange_ensure(has_col(name), "insertion failed: unknown column name");
            map[name] = val;
        }
        std::vector<byte_arr_t> rec;
        for (auto &col: cols) {
            auto it = map.find(col.get_name());
            if (it == map.end()) {
                rec.push_back(col.get_dft());
            } else {
                rec.push_back(it->second);
            }
        }
        insert(rec);
    }

    // 直接插入完整的一条
    void insert(std::vector<byte_arr_t> rec) {
        orange_ensure(rec.size() == cols.size(), "wrong parameter size");
        for (unsigned i = 0; i < rec.size(); i++) {
            orange_ensure(cols[i].test(rec[i]), "column constraints failed");
        }
        insert_internal(rec);
    }

    std::vector<rid_t> single_where(const Orange::parser::single_where& where, rid_t lim) const override {
        if(where.is_null_check()) {
            auto &null = where.null_check();
            return indices[get_col_id(null.col_name)].get_rids_null(null.is_not_null, lim);
        } else if (where.is_op()) {
            auto &op = where.op();
            if (op.expression.is_value()) {
                return indices[get_col_id(op.col_name)].get_rids_value(op.operator_, op.expression.value(), lim);
            } else if (op.expression.is_column()) { // 只会暴力
                std::vector<rid_t> ret;
                auto &col = op.expression.col();
                orange_ensure(!col.table_name.has_value(), "no table name in where clause");
                auto &idx1 = indices[get_col_id(op.col_name)], &idx2 = indices[get_col_id(col.col_name)];
                auto kind1 = find_col(op.col_name)->get_datatype(), kind2 = find_col(col.col_name)->get_datatype();
                for (auto rid: all()) {
                    if (Orange::cmp(idx1.get_val(rid), kind1, op.operator_, idx2.get_val(rid), kind2)) {
                        ret.push_back(rid);
                    }
                }
                return ret;
            } else {
                ORANGE_UNREACHABLE
                return {};
            }
        } else {
            ORANGE_UNREACHABLE
            return {};
        }
    }

    void remove(const std::vector<rid_t>& rids) {
        for (auto &index: indices) {
            for (auto rid: rids) {
                index.remove(rid);
            }
        }
    }

    TmpTable select(std::vector<String> names, const std::vector<rid_t>& rids) {
        TmpTable ret;
        ret.recs.resize(rids.size());
        std::vector<int> col_ids;
        for (auto name : names) {
            auto col_id = get_col_id(name);
            ret.cols.push_back(cols[col_id]);
            auto vals = indices[col_id].get_vals(rids);
            for (unsigned i = 0; i < rids.size(); i++) {
                ret.recs[i].push_back(vals[i]);
            }
        }
        return ret;
    }

    void update(const String& col_name, std::function<byte_arr_t(byte_arr_t)> expr, const std::vector<rid_t>& rids) {
        auto it = find_col(col_name);
        orange_ensure(it != cols.end(), "update failed: unknown column name");
        int col_id = it - cols.begin();
        auto &idx = indices[col_id];
        auto &col = cols[col_id];
        for (auto rid: rids) {
            auto new_val = expr(idx.get_val(rid));
            orange_ensure(col.test(new_val), "update failed: new value fails constraint");
            idx.update(new_val, rid);
        }
    }

    void create_index(const String& col_name) {
        auto it = find_col(col_name);
        orange_ensure(it != cols.end(), "create index failed: unknown column name");
        auto col_id = it - cols.begin();
        indices[col_id].turn_on();
    }

    void drop_index(const String& col_name) {
        auto it = find_col(col_name);
        orange_ensure(it != cols.end(), "drop index failed: unknown column name");
        auto col_id = it - cols.begin();
        indices[col_id].turn_off();
    }

    friend class Index;
};
