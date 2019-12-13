#pragma once

#include "orange/parser/parser.h"
#include "orange/table/table_base.h"

namespace Orange {
    // stmt 返回值类型，为空，为一张表，或者用户操作异常的消息
    // using result_t = boost::variant<boost::blank, TmpTable, String>;
    struct result_t {
        using value_t = boost::variant<boost::blank, TmpTable, String>;
        value_t value;
        
        result_t(const value_t& value) : value(value) {}

        bool ok() const { return value.which() != 2; }
        bool has() const { return value.which() == 1; }
        const TmpTable& get() { return boost::get<TmpTable>(value); }
        const String& what() const { return boost::get<String>(value); }
    };

    // 语义分析入口
    std::vector<result_t> program(Orange::parser::sql_ast& ast);
}  // namespace Orange
