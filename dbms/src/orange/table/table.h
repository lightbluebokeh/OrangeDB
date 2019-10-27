#pragma once

#include <algorithm>
#include <filesystem>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/orange.h>
#include <orange/table/column.h>
#include <orange/table/key.h>
#include <orange/index/index.h>

// 大概是运算的结果的表？
struct table_t {
    std::vector<col_t> cols;
    std::vector<byte_arr_t> recs;
};

// 数据库中的表
class Table {
    int id;
    tbl_name_t name;

    Table(int id, const String& name) : id(id) {
        memset(f_data, 0, sizeof(f_data));
        memset(f_idx, 0, sizeof(f_idx));
        f_pidx = 0;
        f_id = 0;

        memcpy(this->name.data, name.data(), name.length() + 1);
    }
    ~Table() {}

    // metadata
    cnt_t rec_cnt;
    std::vector<col_t> cols;

    p_key_t p_key;
    std::vector<f_key_t> f_keys;

    File *f_data[MAX_COL_NUM], *f_idx[MAX_COL_NUM], *f_pidx, *f_id;
    rid_t stk_top;

    inline String root() { return get_root(name.get()); }
    inline String data_root() { return root() + "data/"; }
    inline String index_root() { return root() + "index/"; }
    inline String metadata_name() { return root() + "metadata.tbl"; }
    inline String data_name(const String& col_name) { return data_root() + col_name + ".db"; }
    inline String index_name(const String& col_name) { return index_root() + col_name + ".idx"; }
    inline String p_index_name() { return root() + "primary.idx"; }
    inline String id_name() { return root() + "id.stk"; }

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

    rid_t new_rid() {
        if (stk_top) {
            return f_id->seek_pos(stk_top-- * sizeof(rid_t))->read<rid_t>();
        } else {
            return rec_cnt++;
        }
    }

    void recycle_id(rid_t rid) { f_id->seek_pos(++stk_top * sizeof(rid_t))->write(rid); }

    // rec 万无一失
    void insert_internal(const std::vector<byte_arr_t>& rec) {
        rid_t rid = new_rid();
        for (unsigned i = 0; i < rec.size(); i++) {
            f_data[i]->seek_pos(rid * cols[i].get_size())->write(rec[i]);
        }
    }

    void open_files() {
        for (unsigned i = 0; i < cols.size(); i++) {
            f_data[i] = File::open(data_name(cols[i].get_name()));
            f_idx[i] = File::open(index_name(cols[i].get_name()));
        }
        f_pidx = File::open(p_index_name());
        f_id = File::open(id_name());
    }

    void close_files() {
        for (unsigned i = 0; i < cols.size(); i++) {
            f_data[i]->close();
            f_idx[i]->close();
        }
        f_pidx->close();
        f_id->close();
    }

    static void free_table(Table* table) {
        if (table == nullptr) return;
        tables[table->id] = nullptr;
        delete table;
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
        table->open_files();
        table->f_id->write(table->stk_top = 0);

        return 1;
    }

    static Table* get(const String& name) {
        check_db();
        ensure(fs::exists(get_root(name)), "table `" + name + "` does not exists");
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (tables[i] && tables[i]->name.data == name) {
                return tables[i];
            }
        }
        auto table = new_table(name);
        table->read_metadata();
        table->open_files();
        return table;
    }

    //  切换数据库的时候调用
    bool close() {
        ensure(this == tables[id], "this is magic");
        write_metadata();
        f_id->seek_pos(0)->write(stk_top);
        close_files();
        free_table(this);
        return 1;
    }

    static bool drop(const String& name) {
        auto table = get(name);
        table->close_files();
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

    void remove(rid_t rid) {
        stk_top++;
        for (unsigned i = 0; i < cols.size(); i++) {
            f_data[i]->seek_pos(cols[i].get_size() * rid)->write<byte_t>(DATA_INVALID);
        }
        f_id->seek_pos(sizeof(rid_t) * rid)->write(rid);
    }

    auto get(rid_t rid) {
        static byte_t bytes[MAX_CHAR_LEN + 2];

        std::vector<byte_arr_t> ret;
        ret.reserve(cols.size());
        for (unsigned i = 0; i < cols.size(); i++) {
            f_data[i]->seek_pos(rid * cols[i].get_size())->read_bytes(bytes, cols[i].get_size());
            ret.push_back(byte_arr_t(bytes, bytes + cols[i].get_size()));
        }
        return ret;
    }

    auto find_col(const String& col_name) {
        auto it = cols.begin();
        while (it != cols.end() && it->get_name() != col_name) it++;
        return it;
    }


    void update(rid_t rid, const String& col_name, byte_arr_t val) {
        auto pos = find_col(col_name);
        if (pos == cols.end()) { 
            throw "yu shi bu jue pao yi chang";
        }

        int id = pos - cols.begin();
        auto rec = get(rid);
        // 确保完整性约束（之类的）
        remove(rid);
        rec[id] = val;
        insert(rec);
    }

    std::vector<rid_t> get_rid(const String& col_name, const byte_arr_t& val) {
        
    }
};
