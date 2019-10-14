#pragma once

#include <string>
#include <vector>
#include <cassert>
#include "fs/fileio/FileManager.h"
#include "fs/utils/pagedef.h"

struct File {
    int id;
    std::string name;

    // name and type
    using DomainDef = std::pair<std::string, std::string>;

    int record_size, record_count;
    std::vector<DomainDef> domains;

    void init_metadata(int record_size, const std::vector<DomainDef>& domains) {
        // TODO
    }

    void load_metadata() {
        // TODO
    }

    static File file[MAX_FILE_NUM];

    static bool create(const std::string& name, int record_size, const std::vector<DomainDef>& domains) {
        int code = FileManager::get_instance()->create_file(name.c_str());
        int id;
        FileManager::get_instance()->open_file(name.c_str(), id);
        file[id].init_metadata(record_size, domains);
        assert(code == 0);
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