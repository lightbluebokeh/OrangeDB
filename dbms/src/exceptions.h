#pragma once

#include "defs.h"

// 返回一些异常信息
namespace Exception {
    inline String no_database_used() { return "use some database first"; }

    inline String database_exists(const String& name) { return "database named `" + name + "` already exists"; }
    inline String database_not_exist(const String& name) { return "database named `" + name + "` does not exist"; }

    inline String table_not_exist(const String& db_name, const String& tbl_name) { return "no table named `" + tbl_name + "` in database `" + db_name + "`"; }
    inline String table_exists(const String& db_name, const String& tbl_name) { return "table named `" + tbl_name + "` already exists in database `" + db_name + "`"; }

    inline String index_not_exist(const String& idx_name, const String& tbl_name) { return "no index named `" + idx_name + "` in table `" + tbl_name + "`"; }
    inline String index_exists(const String& idx_name, const String& tbl_name) { return "index named `" + idx_name + "` already exists in table `" + tbl_name + "`"; }

    inline String col_not_exist(const String& col_name, const String& tbl_name) { return "no column named `" + col_name + "` in table `" + tbl_name + "`"; }
    inline String col_exists(const String& col_name, const String& tbl_name) { return "column named `" + col_name + "` already exists in table `" + tbl_name + "`"; }

    // 尝试删除一个索引正在使用的列
    inline String drop_index_col(const String& col_name, const String& idx_name, const String& tbl_name) { return "cannot drop column `" + col_name + "` in table `" + tbl_name + "` used by index `" + idx_name + "`"; }

    inline String uncomparable(const String& t1, const String& t2) { return "types `" + t1 + "` and `" + t2 + "` are not comparable"; }

    inline String change_nonstring() { return "type change only support for string types(i.e. char and varchar)"; }
    inline String shrink_char() { return "cannot reduce the size of char"; }
    inline String short_varchar() { return "some string is longer than the new limit"; }
};
