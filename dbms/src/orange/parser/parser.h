#pragma once

// 不用sql.h是因为某个系统库也叫sql.h，然后只会include那个而不是本地的
#include "sql_ast.h"
#include <defs.h>
#include <exception>

namespace Orange {
    // 使用 boost::spirit 写的 parser，必须分离cpp不然编译实在是太慢了
    class SqlParser {
    public:
        // 解析sql，语法错误抛异常
        SQL parse(const String& sql);
    };

    struct ParseException : public std::exception {
        int pos;
        
        explicit ParseException(int pos) : std::exception("Parse Error"), pos(pos) {}
    };
}  // namespace Orange
