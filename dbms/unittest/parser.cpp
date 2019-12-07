#include <catch/catch.hpp>
#include <iostream>
#include <optional>
#include <sstream>

#include <defs.h>
#include <orange/parser/parser.h>

using namespace Orange::parser;
static sql_parser parser;

// 生成解析失败的错误信息
std::string generate_error_message(const char* sql, const parse_error& e) {
    std::stringstream ss;
    ss << RED << "FAILED" << RESET << ": " << e.what() << "(at " << e.first << ")\n";
    ss << "  " << sql << '\n';
    ss << "  " << std::string(e.first, ' ') << '^'
       << std::string(std::max(0, e.last - e.first - 1), ' ') << '\n';

    ss << CYAN << "expected" << RESET << ": <" << e.expected << ">\n";
    ss << CYAN << "rest" << RESET << ": '\033[4m" << std::string(sql + e.first, sql + e.last)
       << "\033[0m'\n";
    return ss.str();
}

// 这是无语法错误的sql，需要传入ast
void parse_sql(const char* sql, sql_ast& ast) {
    std::cout << "parsing '\033[4m" << sql << "\033[0m'\n";
    try {
        ast = parser.parse(sql);  // 成功解析
        std::cout << GREEN << "parsed " << ast.stmt_list.size() << " statement"
                  << (ast.stmt_list.size() <= 1 ? "" : "s") << RESET << std::endl;
    }
    catch (const parse_error& e) {
        FAIL(generate_error_message(sql, e));
    }
}
// 包含语法错误的sql
void parse_sql(const char* sql) {
    std::cout << "parsing '\033[4m" << sql << "\033[0m'\n";
    try {
        sql_ast ast = parser.parse(sql);
        // 实际上解析成功了，为什么呢
        FAIL("expecting parse error, but parsed " << ast.stmt_list.size() << " statement"
                                                  << (ast.stmt_list.size() <= 1 ? "" : "s"));
    }
    catch (const parse_error& e) {
        std::cout << YELLOW << "parse error at " << e.first << ": <" << e.expected << "> expected"
                  << RESET << std::endl;
    }
}

/* 开始测试 */

// 测token的识别情况和跳过空格换行符之类的情况
TEST_CASE("Testing keywords and skipper", "[parser]") {
    sql_ast ast;

    // 一些成功的例子
    parse_sql("show DatabaseS;     ", ast);
    parse_sql("create database test1;\ndrop database test2;", ast);
    parse_sql("\tuse test_3_; show tables;", ast);
    parse_sql("select * from table1 where name='测试', name1='中文';", ast);

    // 一些失败的例子
    parse_sql("a b c d");
    parse_sql("tables;");
    parse_sql("showdatabases;");
}

// 测试system statement
TEST_CASE("Testing sys_stmt", "[parser]") {
    sql_ast ast;

    // show databases
    parse_sql("show databases;", ast);
    REQUIRE(ast.stmt_list[0].kind() == StmtKind::Sys);
    REQUIRE(ast.stmt_list[0].sys().kind() == SysStmtKind::ShowDb);
}

// 测试database statement
TEST_CASE("Testing db_stmt", "[parser]") {
    sql_ast ast;

    // create database
    parse_sql("create database test1\t;", ast);
    REQUIRE(ast.stmt_list[0].kind() == StmtKind::Db);
    REQUIRE(ast.stmt_list[0].db().kind() == DbStmtKind::Create);
    REQUIRE(ast.stmt_list[0].db().create().name == "test1");

    // drop database
    parse_sql("drop database test2;", ast);
    REQUIRE(ast.stmt_list[0].db().kind() == DbStmtKind::Drop);
    REQUIRE(ast.stmt_list[0].db().drop().name == "test2");

    // use database
    parse_sql("USE test3;use test4  ;", ast);
    REQUIRE(ast.stmt_list[0].db().kind() == DbStmtKind::Use);
    REQUIRE(ast.stmt_list[0].db().use().name == "test3");
    REQUIRE(ast.stmt_list[1].db().use().name == "test4");

    // show tables
    parse_sql("show tables;", ast);
    REQUIRE(ast.stmt_list[0].db().kind() == DbStmtKind::Show);
}

// 测试table statement
TEST_CASE("Testing tb_stmt", "[parser]") {
    sql_ast ast;

    // create table
    parse_sql("create table a_(col int(3) not);");
    parse_sql("create table aaa("
              "  col1 float not null,"
              "  col2 varchar( 20) default ' abc123中文\t',\n"
              "  col3 int  default NULL"
              ");",
              ast);
    REQUIRE(ast.stmt_list[0].kind() == StmtKind::Tb);
    REQUIRE(ast.stmt_list[0].tb().kind() == TbStmtKind::Create);
    REQUIRE(ast.stmt_list[0].tb().create().name == "aaa");
    REQUIRE(ast.stmt_list[0].tb().create().fields.size() == 3);
    REQUIRE(ast.stmt_list[0].tb().create().fields[0].kind() == FieldKind::Def);
    REQUIRE(ast.stmt_list[0].tb().create().fields[0].def().col_name == "col1");
    REQUIRE(ast.stmt_list[0].tb().create().fields[0].def().type.kind == orange_t::Numeric);
    REQUIRE(ast.stmt_list[0].tb().create().fields[0].def().is_not_null == true);
    REQUIRE(ast.stmt_list[0].tb().create().fields[0].def().default_value.has_value() == false);
    REQUIRE(ast.stmt_list[0].tb().create().fields[1].def().col_name == "col2");
    REQUIRE(ast.stmt_list[0].tb().create().fields[1].def().type.kind == orange_t::Varchar);
    REQUIRE(ast.stmt_list[0].tb().create().fields[1].def().type.int_value() == 20);
    REQUIRE(ast.stmt_list[0].tb().create().fields[1].def().is_not_null == false);
    REQUIRE(ast.stmt_list[0].tb().create().fields[1].def().default_value.get().is_string() == true);
    REQUIRE(ast.stmt_list[0].tb().create().fields[1].def().default_value.get().to_string() ==
            " abc123中文\t");
    REQUIRE(ast.stmt_list[0].tb().create().fields[2].def().col_name == "col3");
    REQUIRE(ast.stmt_list[0].tb().create().fields[2].def().type.kind == orange_t::Int);
    REQUIRE(ast.stmt_list[0].tb().create().fields[2].def().type.has_value() == false);
    REQUIRE(ast.stmt_list[0].tb().create().fields[2].def().is_not_null == false);
    REQUIRE(ast.stmt_list[0].tb().create().fields[2].def().default_value.get().is_null() == true);

    // drop table
    parse_sql("Drop table t1;Drop table t2;drop table t3;", ast);
    REQUIRE(ast.stmt_list.size() == 3);
    REQUIRE(ast.stmt_list[0].tb().kind() == TbStmtKind::Drop);
    REQUIRE(ast.stmt_list[0].tb().drop().name == "t1");
    REQUIRE(ast.stmt_list[1].tb().drop().name == "t2");
    REQUIRE(ast.stmt_list[2].tb().drop().name == "t3");

    // describe table
    parse_sql("desc table0;", ast);
    REQUIRE(ast.stmt_list[0].tb().kind() == TbStmtKind::Desc);

    // insert into table
    parse_sql("insert into table0 values (1, 1.3, '', NULL, 'abc  ');", ast);
    REQUIRE(ast.stmt_list[0].tb().kind() == TbStmtKind::InsertInto);
    REQUIRE(ast.stmt_list[0].tb().insert_into().name == "table0");
    REQUIRE(ast.stmt_list[0].tb().insert_into().columns.has_value() == false);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0].size() == 5);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][0].is_int());
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][0].to_int() == 1);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][1].is_float() == true);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][1].to_float() == 1.3);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][2].is_string() == true);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][2].to_string() == "");
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][3].is_null() == true);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][4].is_string() == true);
    REQUIRE(ast.stmt_list[0].tb().insert_into().values_list[0][4].to_string() == "abc  ");
    parse_sql("insert into table1 (col1, col2) values (123.456, '');", ast);
    REQUIRE(ast.stmt_list[0].tb().insert_into().columns.has_value() == true);
    REQUIRE(ast.stmt_list[0].tb().insert_into().columns.get().size() == 2);
    REQUIRE(ast.stmt_list[0].tb().insert_into().columns.get()[0] == "col1");
    REQUIRE(ast.stmt_list[0].tb().insert_into().columns.get()[1] == "col2");
}

// 测试index statement
TEST_CASE("Testing idx_stmt", "[parser]") {}

// 测试alter statement
TEST_CASE("alter_stmt", "[parser]") {}
