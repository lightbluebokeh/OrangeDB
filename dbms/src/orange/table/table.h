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
    std::vector<rec_t> recs;
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
    std::vector<rec_t> dft;
    size_t col_offset[MAX_COL_NUM];

    p_key_t p_key;
    std::vector<f_key_t> f_keys;

    inline String metadata_name() { return name.get() + "/metadata.tbl"; }

    void write_metadata() {
        if (!fs::exists(metadata_name())) File::create(metadata_name());
        File::open(metadata_name())
            ->seek_pos(0)
            ->write(cols)
            ->write(rec_cnt)
            ->write(p_key)
            ->write(f_keys)
            ->close();

        constexpr int a = sizeof(std::vector<int>);
    }

    void read_metadata() {
        File::open(metadata_name())
            ->seek_pos(0)
            ->read(cols)
            ->read(rec_cnt)
            ->read(p_key)
            ->read(f_keys)
            ->close();
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

public:
    static bool create(const String& name, std::vector<col_t> cols, p_key_t p_key, const std::vector<f_key_t>& f_keys) {
        ensure(!fs::exists(name), "table exists");
        ensure(name.length() <= TBL_NAME_LIM, "table name too long: " + name);

        fs::create_directory(name);
        auto table = new_table(name);
        std::sort(cols.begin(), cols.end(), [] (col_t a, col_t b) { return strncmp(a.name.data, b.name.data, COL_NAME_LIM) < 0; });
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
                delete tables[i];
                tables[i] = nullptr;
                break;
            }
        }
        return fs::remove(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            delete tables[i];
            tables[i] = nullptr;
        }
    }

    bool check_insert() {
        
    }

    void insert(table_t table) {
        
    }
};