#pragma once

#include <string>
#include <vector>
#include <cassert>
#include "fs/fileio/FileManager.h"
#include "fs/utils/pagedef.h"
#include "defs.h"

class File {
public:
    // using DomainDef = std::pair<String, String>;
    struct FieldDef {
        enum TypeKind {
            INT, 
            VARCHAR, 
            FLOAT, 
            DATE
        };

        struct Type {
            TypeKind kind;
            int size;

            static Type parse(String x) {
                Type type;
                if (sscanf(x.c_str(), "VARCHAR(%d)", &type.size) == 1) {
                    type.kind = VARCHAR;
                } else if (sscanf(x.c_str(), "INT(%d)", &type.size) == 1) {
                    type.kind = INT;
                    type.size = 4;
                } else if (x == "FLOAT") {
                    type.kind = FLOAT;
                    type.size = 4;
                } else if (x == "DATE") {
                    type.kind = DATE;
                    type.size = 4;  // YYYY-MM-DD，压位存放
                } else {
                    throw "fail to parse type: " + x;
                }
                return type;
            }
        };
        String name;
        Type type;

        FieldDef(const String& name, String type_raw) : name(name), type(Type::parse(type_raw)) {}
    };
private:
    int id;
    String name;

    int record_size, record_count;
    std::vector<FieldDef> fields;
    int offset[1024];
public:
    void save_metadata(const std::vector<std::pair<String, String>>& name_type_list) {
        for (auto &name_type: name_type_list) {
            
        }
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
        file[id].save_metadata(name_type_list);
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

    static bool remove(const String& name) {
        int code = FileManager::get_instance()->remove_file(name);
        assert(code == 0);
        return 1;
    }
};