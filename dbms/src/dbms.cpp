#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif

#include <orange/syntax/syntax.h>

using namespace Orange;

// 开buffer是因为只能传指针回去，好处是不需要delete了，坏处是多线程就完蛋了
static char buffer[65536];

// 服务端自带多线程，所以这里可能要处理一下
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

    // 返回数据的格式是一个数组，数组每个元素对应每一条语句的执行结果；
    // 一条语句的执行结果包括一个一维数组 "headers"，表头，还有一个二维数组 "data"，代表数据
    // 例子：（假如语句是 "select * from fruit; drop table fruit;"）
    // [
    //   {
    //     "headers": ["名称", "价钱"],
    //     "data": [
    //       ["苹果", 10.5],
    //       ["西瓜", 5],
    //     ],
    //   },
    //   {
    //     "headers": ["操作结果"],
    //     "data": [ ["删除表格成功"] ],
    //   },
    // ]
    // 如果某一条语句失败了，就换成类似于 { "error": "插入失败" }，并且不执行失败语句之后的语句
    // 使用 R"(.....)" 就不需要那么多转移字符了
    sprintf(buffer, R"([{"headers": ["状态"], "data": [["操作完成了，但还没写返回数据的代码"]]}])");
    return buffer;
}
