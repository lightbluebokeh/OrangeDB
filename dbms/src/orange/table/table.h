#pragma once

#include <algorithm>
#include <functional>
#include <filesystem>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/orange.h>
#include <orange/table/column.h>
#include <orange/table/key.h>
#include <orange/index/index.h>
#include <utils/id_pool.h>

// 大概是运算的结果的表？
struct table_t {
    std::vector<col_t> cols;
    std::vector<byte_arr_t> recs;
};

// 数据库中的表
class Table {
    int id;
    // tbl_name_t name;
    String name;
    IdPool<rid_t> rid_pl;

    Table(int id, const String& name) : id(id), name(name), rid_pl(pool_name()) {}
    ~Table() {}

    // metadata
    cnt_t rec_cnt;
    std::vector<col_t> cols;
    p_key_t p_key;
    std::vector<f_key_t> f_keys;

    std::vector<Index> idxs;

    inline String root() { return get_root(name) + "/"; }
    inline String data_root() { return root() + "data/"; }
    inline String metadata_name() { return root() + "metadata.tbl"; }
    inline String pool_name() { return root() + "rid.pl"; }

    void write_metadata() {
        File::open(metadata_name())->seek_pos(0)->write(cols, rec_cnt, p_key, f_keys)->close();
    }
    void read_metadata() {
        File::open(metadata_name())->seek_pos(0)->read(cols, rec_cnt, p_key, f_keys)->close();
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
        return (Table*)0x1234567890;
    }

    static void check_db() { ensure(Orange::using_db(), "use some database first"); }

    // rec 万无一失
    void insert_internal(const std::vector<byte_arr_t>& rec) {
        rid_t rid = rid_pl.new_id();
        for (unsigned i = 0; i < rec.size(); i++) {
            idxs[i].insert(rec[i], rid);
        }
        rec_cnt++;
    }

    static void free_table(Table* table) {
        if (table == nullptr) return;
        tables[table->id] = nullptr;
        delete table;
    }

    void on_create(std::vector<col_t> cols, p_key_t p_key, const std::vector<f_key_t>& f_keys) {
        std::sort(cols.begin(), cols.end(), [](col_t a, col_t b) { return a.get_name() < b.get_name(); });
        this->cols = std::move(cols);
        this->p_key = p_key;
        this->f_keys = f_keys;
        this->rec_cnt = 0;
        write_metadata();
        rid_pl.init();
        for (auto col: cols) {
            idxs.push_back(Index(col.get_size(), data_root()));
            if (col.is_indexed()) idxs.back().turn_on();
        }
    }
public:
    static String get_root(const String& name) { return "[" + name + "]/"; }

    static bool create(const String& name, std::vector<col_t> cols, p_key_t p_key,
                       const std::vector<f_key_t>& f_keys) {
        check_db();
        ensure(name.length() <= TBL_NAME_LIM, "table name too long: " + name);

        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        std::sort(cols.begin(), cols.end(),
                  [](col_t a, col_t b) { return a.get_name() < b.get_name(); });
        table->cols = std::move(cols);
        table->p_key = p_key;
        table->f_keys = f_keys;
        table->rec_cnt = 0;
        table->write_metadata();
        table->rid_pl.init();
        for (auto col: cols) {
            table->idxs.push_back(Index(col.get_size(), table->data_root()));
        }
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
        table->rid_pl.load();
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

    auto find_col(const String& col_name) {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        return it;
    }

    // 这一段代码把输入的值补全
    void insert(std::vector<std::pair<byte_arr_t, String>> val_name_list) {
        sort(val_name_list.begin(), val_name_list.end(),
             [](auto a, auto b) { return a.second < b.second; });
        std::vector<byte_arr_t> rec;
        auto it = cols.begin();
        for (auto [val, name] : val_name_list) {
            if (it != cols.end() && it->get_name() > name) continue;
            while (it != cols.end() && it->get_name() < name) {
                ensure(it->has_dft(), "no default value for non-null column: " + it->get_name());
                rec.push_back(it->get_dft());
                it++;
            }
            if (it != cols.end() && it->get_name() == name) {
                ensure(!it->adjust(val), "column constraints failed");
                rec.push_back(val);
                it++;
                while (it != cols.end() && it->get_name() == (it - 1)->get_name()) it++;
            }
        }
        while (it != cols.end()) {
            ensure(it->has_dft(), "no default value for non-null column: " + it->get_name());
            rec.push_back(it->get_dft());
            it++;
        }
        insert_internal(rec);
    }

    void insert(std::vector<byte_arr_t> rec) {
        ensure(rec.size() == cols.size(), "wrong parameter size");
        for (unsigned i = 0; i < rec.size(); i++) {
            ensure(cols[i].adjust(rec[i]), "column constraints failed");
        }
        insert_internal(rec);
    }

    void remove(WhereClause wc) {
        auto it = find_col(wc.col_name);
        ensure(it != cols.end(), "where clause failed");
        int col_id = it - cols.begin();
        cols[col_id].adjust(wc.val);
        auto &idx = idxs[col_id];
        for (auto rid: idx.get_rid(wc.cmp, wc.val)) {
            idx.remove(rid);
            rec_cnt--;
        }
    }

    auto select(std::vector<String> names, WhereClause wc) {
        auto it = find_col(wc.col_name);
        ensure(it != cols.end(), "where clause failed");
        int col_id = it - cols.begin();
        cols[col_id].adjust(wc.val);
        std::vector<std::vector<byte_arr_t>> ret;
        std::vector<int> col_ids;
        for (auto name: names) {
            auto it = find_col(name);
            ensure(it != cols.end(), "???");
            col_ids.push_back(it - cols.begin());
        }
        for (auto rid: idxs[col_id].get_rid(wc.cmp, wc.val)) {
            std::vector<byte_arr_t> cur;
            for (auto cid: col_ids) {
                cur.push_back(idxs[cid].get_val(rid));
            }
            ret.push_back(cur);
        }
        return ret;
    }

    void update(std::function<byte_arr_t(byte_arr_t)> expr, WhereClause wc) {
        auto it = find_col(wc.col_name);
        ensure(it != cols.end(), "where clause failed");
        int col_id = it - cols.begin();
        cols[col_id].adjust(wc.val);
        auto &idx = idxs[col_id];
        for (auto rid: idx.get_rid(wc.cmp, wc.val)) {
            idx.update(expr(idx.get_val(rid)), rid);
        }
    }
};
