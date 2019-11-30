#pragma once

#include <orange/parser/parser.h>

namespace Orange {
    // 语义分析入口
    void program(Orange::parser::sql_ast& ast);
}  // namespace Orange
