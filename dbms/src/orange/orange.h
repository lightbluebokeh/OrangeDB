#pragma once

#include <defs.h>
#include <unordered_set>

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
    std::vector<String> all();
    std::vector<String> all_tables();
};  // namespace Orange
