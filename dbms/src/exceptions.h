#pragma once

#include "defs.h"

// 返回一些异常信息
namespace Exception {
    inline String no_database_used() { return "use some database first"; }
    inline String database_exists(const String& name) { return "database named `" + name + "` already exists"; }
    inline String database_not_exist(const String& name) { return "database named `" + name + "` does not exist"; }
    inline String table_not_exist(const String& db_name, const String& tbl_name) { return "there is not table named `" + tbl_name + "` in database `" + db_name + "`"; }
    inline String table_exists(const String& db_name, const String& tbl_name) { return "there is not table named `" + tbl_name + "` in database `" + db_name + "`"; }
};
