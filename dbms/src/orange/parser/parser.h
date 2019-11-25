#pragma once

// 不用sql.h是因为某个系统库也叫sql.h，然后只会include那个而不是本地的
#include "sql_ast.h"
#include <defs.h>
#include <exception>

namespace Orange {
    namespace parser {
        // 使用 boost::spirit 写的 parser
        class sql_parser {
        public:
            // 解析sql，语法错误抛异常
            sql_ast parse(const String& sql);
        };

        struct ParseException : public std::exception {
            int pos;

            explicit ParseException(int pos) : std::exception("Parse Error"), pos(pos) {}
        };
    }  // namespace parser
}  // namespace Orange
