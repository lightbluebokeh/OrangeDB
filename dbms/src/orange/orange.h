#pragma once

#include "defs.h"
#include "orange/table/table_base.h"

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
    String get_cur();
    // 所有数据库
    std::vector<String> all();  
    std::vector<String> all_tables();
};  // namespace Orange
