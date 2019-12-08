#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif

#include <orange/syntax/syntax.h>

using namespace Orange;

static char buffer[65536];

// 只能单线程
API const char* exec(const char* sql) {
    parser::sql_parser sql_parser;

    parser::sql_ast ast;
    try {
        ast = sql_parser.parse(sql);
    }
    catch (const parser::parse_error& e) {
        sprintf(buffer, R"([{"error": "在第 %d 个字符附近有语法错误"}])", e.first);
        return buffer;
    }

    program(ast);

    sprintf(buffer, R"([{"headers": ["状态"], "data": [["操作完成了，但还没写返回数据的代码"]]}])");
    return buffer;
}
