#pragma once

#include <filesystem>

#include "defs.h"
#include "orange/table/table_base.h"

// root of database files

namespace Orange {
    void setup();
    void paolu();
    bool exists(const String& name);
    bool create(const String& name);
    bool drop(const String& name);
    bool use(const String& name);
    //  正在使用某个数据库
    bool using_db();
    bool unuse();
    // 返回当前使用的数据库的路径
    fs::path cur_db_path();
    // 所有数据库
    std::vector<String> all();  
    std::vector<String> all_tables();
};  // namespace Orange
