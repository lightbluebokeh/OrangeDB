#pragma once

#include "fs/fileio/FileManager.h"
#include "fs/utils/pagedef.h"
#include <cassert>
#include <cstring>
#include <defs.h>
#include <fs/bufmanager/BufPageManager.h>
#include <fs/bufmanager/buf_page.h>
#include <fs/bufmanager/buf_page_stream.h>
#include <record/filed_def.h>
#include <string>
#include <vector>

// 打开的文件
class File {
private:
    int id;
    String name;

    // file's metadata
    int record_size, record_cnt;
    std::vector<FieldDef> fields;

    File(int id, const String& name) : id(id), name(name) {}
    File(const File&) = delete;

    ~File() {}

    void init_metadata(const std::vector<raw_field_t>& raw_fields) {
        ensure(raw_fields.size() <= MAX_COL_NUM, "to much fields");
        fields.clear();
        BufPageStream bps(get_buf_page(0));

        record_size = 0;
        bps.write<byte_t>(raw_fields.size());
        for (auto raw_field : raw_fields) {
            auto field = FieldDef::parse(raw_field);
            bps.write(field);
            record_size += field.get_size();
            fields.push_back(std::move(field));
        }
        bps.memset(0, sizeof(FieldDef) * (MAX_COL_NUM - fields.size()))
            .write<uint16_t>(record_size)
            .write(record_cnt = 0)
            .memset();
    }

    void load_metadata() {
        fields.clear();
        BufPageStream bps(get_buf_page(0));

        fields.reserve(bps.read<int>());
        for (size_t i = 0; i < fields.capacity(); i++) {
            fields.push_back(bps.read<FieldDef>());
        }
        bps.seekoff(sizeof(FieldDef) * (MAX_COL_NUM - fields.size()))
            .read<uint16_t>(record_size)
            .read<int>(record_cnt);
    }

    BufPage get_buf_page(int page_id) { return BufPage(id, page_id); }

    static File* file[MAX_FILE_NUM];
    static void close_internal(int id) {
        delete file[id];
        file[id] = nullptr;
    }

public:
    static bool create(const std::string& name, const std::vector<raw_field_t>& raw_fields) {
        auto fm = FileManager::get_instance();
        int code = fm->create_file(name.c_str());
        ensure(code == 0, "file create fail");
        int id;
        code = fm->open_file(name.c_str(), id);
        ensure(code == 0, "file open failed");
        file[id] = new File(id, name);
        file[id]->init_metadata(raw_fields);
        code = fm->close_file(id);
        ensure(code == 0, "file close failed");
        close_internal(id);
        return 1;
    }

    // 不存在就错误了
    static File* open(const String& name) {
        auto fm = FileManager::get_instance();
        int id;
        int code = fm->open_file(name.c_str(), id);
        ensure(code == 0, "file open failed");
        if (file[id] == nullptr) {
            file[id] = new File(id, name);
            file[id]->load_metadata();
        }
        return file[id];
    }

    // 调用后 f 变成野指针
    static bool close(File* f) {
        auto fm = FileManager::get_instance();
        int code = fm->close_file(f->id);
        ensure(code == 0, "file close failed");
        ensure(file[f->id] == f, "this is magic");
        close_internal(f->id);
        return 1;
    }

    static bool remove(const String& name) {
        auto fm = FileManager::get_instance();
        int code = fm->remove_file(name);
        ensure(code == 0, "remove file failed");
        return 1;
    }

    String get_name() { return this->name; }
};