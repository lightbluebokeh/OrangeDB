#pragma once

#include <filesystem>
#include <algorithm>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/orange.h>
#include <orange/table/key.h>
#include <orange/table/column.h>

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
        memcpy(this->name.data, name.data(), name.length());
    }
    ~Table() {}

    // metadata
    int rec_cnt;
    std::vector<col_t> cols;
    std::vector<byte_arr_t> dft;
    size_t col_offset[MAX_COL_NUM];

    p_key_t p_key;
    std::vector<f_key_t> f_keys;

    inline String prefix() { return name.get() + "/"; }
    inline String metadata_name() { return prefix() + "metadata.tbl"; }
    inline String data_name(const String& col_name) { return prefix() + "data/" + col_name + ".db";  } 
    inline String index_name(const String& col_name) { return prefix() + "index/" + col_name + ".idx"; }
    inline String p_index_name() { return prefix() + "primary.idx"; }
    inline String id_name() { return prefix() + "id.stk"; }

    void write_metadata() {
        if (!fs::exists(metadata_name())) File::create(metadata_name());
        File::open(metadata_name())->seek_pos(0)
            ->write(cols, rec_cnt, dft, p_key, f_keys);
    }

    void read_metadata() {
        File::open(metadata_name())->seek_pos(0)
            ->read(cols, rec_cnt, dft, p_key, f_keys);
    }

    static Table *tables[MAX_TBL_NUM];

    static Table* new_table(const String& name) {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (!tables[i]) {
                tables[i] = new Table(i, name);
                tables[i]->id = i;
                return tables[i];
            }
        }
        throw "number of tables exceeded";
        return (Table*)0x1234567890;
    }

    static void check_db() { ensure(Orange::using_db(), "use some database first"); }

    void insert_internal(const std::vector<byte_arr_t>& rec) {

    }

public:
    static bool create(const String& name, std::vector<col_t> cols, p_key_t p_key, const std::vector<f_key_t>& f_keys) {
        ensure(!fs::exists(name), "table exists");
        ensure(name.length() <= TBL_NAME_LIM, "table name too long: " + name);

        fs::create_directory(name);
        auto table = new_table(name);
        std::sort(cols.begin(), cols.end(), [] (col_t a, col_t b) { return a.get_name() < b.get_name(); });
        table->cols = std::move(cols);
        table->p_key = p_key;
        table->f_keys = f_keys;
        table->rec_cnt = 0;
        table->write_metadata();

        return 1;
    }

    static bool exists(const String& name) {
        check_db();
        return fs::exists(name);
    }

    static Table* open(const String& name) {
        check_db();
        ensure(fs::exists(name), "table[" + name + "] does not exists in database [" + Orange::get_cur() + "]");
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (tables[i] && tables[i]->name.data == name) {
                // 或者还是抛个异常？ 
                return tables[i];
            }
        }
        
        auto table = new_table(name);
        table->read_metadata();
        return table;
    }

    bool close() {
        ensure(this == tables[id], "this is magic");
        write_metadata();
        File::close(metadata_name());
        for (auto& col: cols) {
            File::close(data_name(col.get_name()));
            File::close(index_name(col.get_name()));
        }
        File::close(p_index_name());
        File::close(id_name());

        tables[id] = nullptr;
        delete this;
        return 1;
    }

    static bool drop(const String& name) {
        check_db();
        ensure(fs::exists(name), "table [" + name + "] does not exists");

        // 写暴力真爽
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (tables[i] && tables[i]->name.data == name) {
                ensure(tables[i]->close(), "close table failed");
                break;
            }
        }
        return fs::remove(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (auto table: tables) {
            if (table) ensure(table->close(), "close table failed");
        }
    }

    void insert(std::vector<std::pair<byte_arr_t, String>> val_name_list) {
        sort(val_name_list.begin(), val_name_list.end(), [] (auto a, auto b) { return a.second < b.second; });
        std::vector<byte_arr_t> rec;
        auto it = cols.begin();
        for (auto [val, name]: val_name_list) {
            if (it != cols.end() && it->get_name() > name) continue;
            while (it != cols.end() && it->get_name() < name) {
                ensure(it->has_dft(), "no default value for non-null column: " + it->get_name());
                rec.push_back(it->get_dft());
                it++;
            }
            if (it != cols.end() && it->get_name() == name) {
                ensure(!it->ajust(val), "column constraints failed");
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
            ensure(cols[i].ajust(rec[i]), "column constraints failed");
        }
        insert_internal(rec);
    }
};
