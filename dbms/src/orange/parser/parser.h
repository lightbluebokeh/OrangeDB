#pragma once

#include "sql_ast.h"
#include <defs.h>
#include <exception>


namespace Orange {
    namespace parser {
        // 使用 boost::spirit 写的 parser
        class sql_parser {
        public:
            sql_ast parse(const std::string& sql);
        };

        // 解析错误抛异常
        struct parse_error : public std::runtime_error {
            // first:错误位置
            int first, last;  // 由于使用迭代器来解析，last会始终指向字符串结尾，先留着说不定可扩展

            // 期望的非终结符，目前仅能调试用，release模式下全都变成<unnamed-rule>，等搞懂框架抛的异常才能返回真的expected
            std::string expected;

            parse_error(const char* msg, int first, int last, const std::string& expected) :
                std::runtime_error(msg), first(first), last(last), expected(expected) {}
        };
    }  // namespace parser
}  // namespace Orange
