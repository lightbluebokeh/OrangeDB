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
    ~SavedTable() {}

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
        File::open(metadata_name())->seek_pos(0)->write(cols, rec_cnt, p_key, f_keys)->close();
    }
    void read_metadata() {
        File::open(metadata_name())->seek_pos(0)->read(cols, rec_cnt, p_key, f_keys)->close();
        for (auto &col: cols) {
            indices.emplace_back(Index(*this, col.get_datatype(), col.key_size(), col_prefix(col.get_name()), col.has_index()));
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
        throw "cannot allocate table";
    }

    static void check_db() { ensure(Orange::using_db(), "use some database first"); }

    // rec 万无一失
    void insert_internal(const std::vector<byte_arr_t>& rec) {
        rid_t rid = rid_pool.new_id();
        for (unsigned i = 0; i < rec.size(); i++) {
            indices[i].insert(rec[i], rid);
        }
        rec_cnt++;
    }

    static void free_table(SavedTable* table) {
        if (table == nullptr) return;
        tables[table->id] = nullptr;
        delete table;
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
        for (auto col: this->cols) {
            indices.emplace_back(Index(*this, col.get_datatype(), col.key_size(), col_prefix(col.get_name()), col.has_index()));
        }
    }

    static String get_root(const String& name) { return name + "/"; }
protected:
    std::vector<rid_t> all() const override { return rid_pool.all(); }

    byte_arr_t get_field(rid_t rid, int col_id) const override { return this->indices[col_id].get_val(rid); }
public:
    static bool create(const String& name, const std::vector<Column>& cols, const std::vector<String>& p_key,
                       const std::vector<f_key_t>& f_keys) {
        check_db();
        // ensure(name.length() <= TBL_NAME_LIM, "table name too long: " + name);
        ensure(!fs::exists(get_root(name)), "table `" + name + "' exists");
        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        table->on_create(cols, p_key, f_keys);
        return 1;
    }

    static SavedTable* get(const String& name) {
        check_db();
        ensure(fs::exists(get_root(name)), "table `" + name + "` does not exists");
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (tables[i] && tables[i]->name == name) {
                return tables[i];
            }
        }
        auto table = new_table(name);
        table->read_metadata();
        table->rid_pool.load();
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

    //  切换数据库的时候调用
    bool close() {
        ensure(this == tables[id], "this is magic");
        write_metadata();
        free_table(this);
        return 1;
    }

    static bool drop(const String& name) {
        auto table = get(name);
        table->close();
        free_table(table);
        return fs::remove_all(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (auto table : tables) {
            if (table) ensure(table->close(), "close table failed");
        }
    }

    // 这一段代码尝试把输入的值补全
    void insert(const std::vector<std::pair<String, byte_arr_t>>& values) {
        std::unordered_map<String, byte_arr_t> map;
        for (auto [name, val]: values) {
            ensure(has_col(name), "insertion failed: unknown column name");
            map[name] = val;
        }
        std::vector<byte_arr_t> rec;
        for (auto &col: cols) {
            auto it = map.find(col.get_name());
            if (it == map.end()) {
                ensure(col.has_dft(), "insertion failed: no default value for unspecified column: " + col.get_name());
                rec.push_back(col.get_dft());
            } else {
                ensure(col.test(it->second), "insertion failed: value fails constraints");
                rec.push_back(it->second);
            }
        }
        insert_internal(rec);
    }

    void insert(std::vector<byte_arr_t> rec) {
        ensure(rec.size() == cols.size(), "wrong parameter size");
        for (unsigned i = 0; i < rec.size(); i++) {
            ensure(cols[i].test(rec[i]), "column constraints failed");
        }
        insert_internal(rec);
    }
    
    std::vector<rid_t> where(const String& col_name, pred_t pred, rid_t lim) {
        auto it = find_col(col_name);
        ensure(it != cols.end(), "where clause failed: unknown column name");
        int col_id = it - cols.begin();
        auto &col = cols[col_id];
        ensure(col.test(pred.lo) && col.test(pred.hi), "bad where clause");
        return indices[col_id].get_rid(pred, lim);
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
            auto col_id = find_col(name) - cols.begin();
            for (unsigned i = 0; i < rids.size(); i++) {
                ret.recs[i].push_back(this->indices[col_id].get_val(rids[i]));
            }
        }
        return ret;
    }

    void update(const String& col_name, std::function<byte_arr_t(byte_arr_t)> expr, const std::vector<rid_t>& rids) {
        auto it = find_col(col_name);
        ensure(it != cols.end(), "update failed: unknown column name");
        int col_id = it - cols.begin();
        auto &idx = indices[col_id];
        auto &col = cols[col_id];
        for (auto rid: rids) {
            auto new_val = expr(idx.get_val(rid));
            ensure(col.test(new_val), "update failed: new value fails constraint");
            idx.update(new_val, rid);
        }
    }

    void create_index(const String& col_name) {
        auto it = find_col(col_name);
        ensure(it != cols.end(), "create index failed: unknown column name");
        auto col_id = it - cols.begin();
        indices[col_id].turn_on();
    }

    void drop_index(const String& col_name) {
        auto it = find_col(col_name);
        ensure(it != cols.end(), "drop index failed: unknown column name");
        auto col_id = it - cols.begin();
        indices[col_id].turn_off();
    }

    friend class Index;
};
