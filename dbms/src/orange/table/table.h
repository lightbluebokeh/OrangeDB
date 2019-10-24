#pragma once

#include <defs.h>
#include <orange/orange.h>
#include <filesystem>
#include <fs/file/file.h>
#include <orange/table/column.h>
#include <orange/table/key.h>

class Table {
    int id;
    tbl_name_t name;

    Table(int id, const String& name) : id(id) {
        memcpy(this->name.data, name.data(), name.length());
    }
    ~Table() {}

    // metadata
    int record_size, record_cnt;
    std::vector<col_t> cols;
    p_key_t p_key;
    std::vector<f_key_t> f_keys;

    inline String metadata_name() { return String(name.data) + "/metadata.txt"; }
    inline String data_name() { return String(name.data) + "/data.txt"; }

    void write_metadata() {
        if (!fs::exists(metadata_name())) File::create(metadata_name());

        auto file = File::open(metadata_name());
        file->seek_pos(0);

        file->write(cols.size(), 1);
        for (auto col: cols) {
            file->write(col);
        }
        record_size = 0;
        for (auto col: cols) record_size += col.get_size();
        file->write(record_size)
            ->write(record_cnt)
            ->write(p_key)
            ->write(f_keys.size(), 4);
        for (auto f_key: f_keys) {
            file->write(f_key);
        }

    }

    void read_metadata() {
        auto file = File::open(metadata_name());
        file->seek_pos(0);
        cols.clear();
        cols.reserve(file->read<byte_t>());
        for (int i = 0; i < (int)cols.size(); i++) {
            cols.push_back(file->read<col_t>());
        }
        file->read(record_size)
            ->read(record_cnt)
            ->read(p_key);

        f_keys.clear();
        f_keys.reserve(file->read<int>());
        for (int i = 0; i < (int)f_keys.capacity(); i++) {
            f_keys.push_back(file->read<f_key_t>());
        }
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
    static bool create(const String& name, const std::vector<col_t>& cols, p_key_t p_key, const std::vector<f_key_t>& f_keys) {
        ensure(!fs::exists(name), "table exists");
        ensure(name.length() <= TBL_NAME_LIM, "table name too long: " + name);

        fs::create_directory(name);
        auto table = new_table(name);
        table->cols = cols;
        table->p_key = p_key;
        table->f_keys = f_keys;
        table->record_cnt = 0;
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

        auto table = new_table(name);
        table->read_metadata();
        return table;
    }

    bool close() {
        write_metadata();
        return 1;
    }

    bool drop() {
        namespace fs = std::filesystem;
        ensure(this == tables[id], "this is magic");
        fs::remove(name.data);
        tables[id] = nullptr;
        delete this;
        return 1;
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            delete tables[i];
            tables[i] = nullptr;
        }
    }
};