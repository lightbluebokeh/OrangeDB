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
#include <utils/id_pool.h>

// 大概是运算的结果的表？
struct table_t {
    std::vector<col_t> cols;
    std::vector<byte_arr_t> recs;
};

// 数据库中的表
class Table {
    int id;
    String name;
    IdPool<rid_t> rid_pool;

    Table(int id, const String& name) : id(id), name(name), rid_pool(pool_name()) {}
    ~Table() {}

    // metadata
    rid_t rec_cnt;
    std::vector<col_t> cols;
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
            indices.emplace_back(Index(*this, col.key_kind(), col.get_size(), col_prefix(col.get_name()), col.has_index()));
            indices.back().load();
        }
    }

    static Table* tables[MAX_TBL_NUM];

    static Table* new_table(const String& name) {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (!tables[i]) {
                tables[i] = new Table(i, name);
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

    static void free_table(Table* table) {
        if (table == nullptr) return;
        tables[table->id] = nullptr;
        delete table;
    }

    void on_create(const std::vector<col_t>& cols, const std::vector<String>& p_key, const std::vector<f_key_t>& f_keys) {
        this->cols = cols;
        this->p_key = p_key;
        this->f_keys = f_keys;
        this->rec_cnt = 0;
        File::create(metadata_name());
        write_metadata();
        rid_pool.init();
        fs::create_directory(data_root());
        for (auto col: this->cols) {
            indices.emplace_back(Index(*this, col.key_kind(), col.get_size(), data_root() + name, col.has_index()));
        }
    }

    auto find_col(const String& col_name) {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        return it;
    }
    bool has_col(const String& col_name) { return find_col(col_name) != cols.end(); }

    static String get_root(const String& name) { return name + "/"; }
public:
    static bool create(const String& name, const std::vector<col_t>& cols, const std::vector<String>& p_key,
                       const std::vector<f_key_t>& f_keys) {
        check_db();
        ensure(name.length() <= TBL_NAME_LIM, "table name too long: " + name);
        ensure(!fs::exists(get_root(name)), "table `" + name + "' exists");
        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        table->on_create(cols, p_key, f_keys);
        return 1;
    }

    static Table* get(const String& name) {
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

    //  切换数据库的时候调用
    bool close() {
        ensure(this == tables[id], "this is magic");
        write_metadata();
        free_table(this);
        return 1;
    }

    static bool drop(const String& name) {
        auto table = get(name);
        // table->close_files();
        table->close();
        free_table(table);
        return fs::remove(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (auto table : tables) {
            if (table) ensure(table->close(), "close table failed");
        }
    }

    // 这一段代码把输入的值补全
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

    auto select(std::vector<String> names, const std::vector<rid_t>& rids) {
        std::vector<std::vector<byte_arr_t>> ret;
        std::vector<int> col_ids;
        for (auto name : names) {
            auto it = find_col(name);
            ensure(it != cols.end(), "???");
            col_ids.push_back(int(it - cols.begin()));
        }
        for (auto rid: rids) {
            std::vector<byte_arr_t> cur;
            for (auto cid : col_ids) {
                cur.push_back(indices[cid].get_val(rid));
            }
            ret.push_back(cur);
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
        indices[it - cols.begin()].turn_on();
    }

    void drop_index(const String& col_name) {
        auto it = find_col(col_name);
        ensure(it != cols.end(), "drop index failed: unknown column name");
        indices[it - cols.begin()].turn_off();
    }

    friend class Index;
};
