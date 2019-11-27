#include <catch/catch.hpp>
#include <iostream>
#include <optional>

#include <defs.h>
#include <orange/parser/parser.h>

using namespace Orange::parser;
static sql_parser parser;

// 生成解析失败的错误信息
std::string generate_error_message(const char* sql, const parse_error& e) {
    std::stringstream ss;
    ss << RED << "FAILED" << RESET << ": " << e.what() << "(at " << e.first << ")\n";
    ss << "  " << sql << '\n';
    ss << "  " << std::string(e.first, ' ') << '^' << std::string(e.last - e.first - 1LL, ' ')
       << '\n';

    ss << CYAN << "expected" << RESET << ": " << e.expected << '\n';
    ss << CYAN << "got" << RESET << ": '" << std::string(sql + e.first, sql + e.last) << "'\n";
    return ss.str();
}

// 封装测试的宏，主要是让INFO和FAIL出现在同一个block里
void parse_sql(const char* sql, sql_ast& ast) {
    std::cout << "parsing '" << sql << "'\n";
    try {
        ast = parser.parse(sql);  // 成功解析
        std::cout << GREEN << "parsed " << ast.stmt.size() << " statements" << RESET << std::endl;
    }
    catch (const parse_error& e) {
        FAIL(generate_error_message(sql, e));
    }
}
void parse_sql(const char* sql) {
    std::cout << "parsing '" << sql << "'\n";
    try {
        sql_ast ast = parser.parse(sql);  // 解析失败
        FAIL("parse error expected, parsed " << ast.stmt.size() << " statements");
    }
    catch (const parse_error& e) {
        std::cout << RED << "parse error at " << e.first << RESET << ": expecting " << e.expected
                  << std::endl;
    }
}

TEST_CASE("syntax", "[parser]") {
    sql_ast ast;

    // 一些成功的例子
    parse_sql("show Databases;", ast);
    parse_sql("  create\ndatabase test1;\ndrop database\t\n\ttest2;", ast);
    parse_sql("\tuse test_3_; show tables;\n ", ast);

    // 一些失败的例子
    parse_sql("SHOW databases");
    parse_sql("a b c d");
    parse_sql("tables;");
    parse_sql("create database 1a;");
    parse_sql("drop table 测试;");
    parse_sql("showdatabases;");
}

TEST_CASE("sys_stmt", "[parser]") {
    using boost::get;
    sql_ast ast;

    // show databases
    parse_sql("show databases;", ast);
    REQUIRE(ast.stmt.size() == 1);
    REQUIRE(ast.stmt[0].which() == (int)StmtKind::Sys);
    REQUIRE(get<sys_stmt>(ast.stmt[0]).which() == (int)SysStmtKind::ShowDb);


    /* db_stmt */

    parse_sql("create database test1;drop database test2;", ast);
    REQUIRE(ast.stmt.size() == 2);
    // create database
    REQUIRE(ast.stmt[0].which() == (int)StmtKind::Db);
    REQUIRE(get<db_stmt>(ast.stmt[0]).which() == (int)DbStmtKind::Create);
    REQUIRE(get<create_db_stmt>(get<db_stmt>(ast.stmt[0])).name == "test1");
    // drop database
    REQUIRE(ast.stmt[1].which() == (int)StmtKind::Db);
    REQUIRE(get<db_stmt>(ast.stmt[1]).which() == (int)DbStmtKind::Drop);
    REQUIRE(get<drop_db_stmt>(get<db_stmt>(ast.stmt[1])).name == "test2");


    /* tb_stmt */

    parse_sql("create table aaa (col1 float not null, col2 varchar(2));", ast);
    REQUIRE(ast.stmt.size() == 1);
    // create table
    REQUIRE(ast.stmt[0].which() == (int)StmtKind::Tb);
    REQUIRE(get<tb_stmt>(ast.stmt[0]).which() == (int)TbStmtKind::Create);
    REQUIRE(get<create_tb_stmt>(get<tb_stmt>(ast.stmt[0])).name == "aaa");
}
