#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <defs.h>
#include "fs/fileio/FileManager.h"
#include "fs/utils/pagedef.h"
#include <record/filed_def.h>
#include <fs/bufmanager/BufPageManager.h>
#include <record/bytes.h>

class File {
private:
    int id;
    std::string name;

    int record_size, record_count;
    std::vector<FieldDef> fields;

    // name and type
    void init_metadata(const std::vector<std::pair<String, String>>& name_type_list) {
        assert(name_type_list.size() <= MAX_COL_NUM);
        fields.clear();
        int page_id;
        auto buf = BufPageManager::get_instance()->getPage(id, 0, page_id);
        // memset(buf, name_type_list.size(), 1);
        buf++;
        for (auto &name_type: name_type_list) {
            auto field = FieldDef::parse(name_type);
            // write_buf<Bytes>(buf, field.to_bytes(), COL_SIZE);
            // write_buf()
            // write_buf(buf, )==
            // auto bytes = field.to_bytes();
            // memcpy(buf, bytes.data(), bytes.length());
            // memcpy(buf, bytes.data() + bytes.length(), COL_SIZE - bytes.length());
            fields.push_back(std::move(field));
            buf += COL_SIZE;
        }
        memset(buf, 0, COL_SIZE * (MAX_COL_NUM - fields.size()));
        buf += COL_SIZE * (MAX_COL_NUM - fields.size());
        
    }

    void load_metadata() {
        // TODO
    }

    static File file[MAX_FILE_NUM];
public:
    static bool create(const std::string& name, int record_size, const std::vector<std::pair<String, String>>& name_type_list) {
        int code = FileManager::get_instance()->create_file(name.c_str());
        assert(code == 0);
        int id;
        FileManager::get_instance()->open_file(name.c_str(), id);
        // file[id].init_metadata(record_size, domains);
        file[id].init_metadata(name_type_list);
        assert(code == 0);
        FileManager::get_instance()->close_file(id);
        return 1;
    }

    static File* open(const std::string& name) {
        int id;
        int code = FileManager::get_instance()->open_file(name.c_str(), id);
        assert(code == 0);
        file[id].id = id;
        file[id].name = name;
        file[id].load_metadata();
        return &file[id];
    }

    static bool close(int id) {
        int code = FileManager::get_instance()->close_file(id);
        assert(code == 0);
        return 1;
    }

    static bool remove(const std::string& name) {
        int code = FileManager::get_instance()->remove_file(name);
        assert(code == 0);
        return 1;
    }
};