#include <catch/catch.hpp>
#include <iostream>
#include <optional>

#include <defs.h>
#include <orange/parser/parser.h>

using namespace Orange::parser;

std::string generate_error_message(const parse_error& e) {
    std::stringstream ss;
    ss << "FAILED: " << e.what() << " at " << e.first << '\n';
    ss << "    " << sql << '\n';
    ss << "    ";
    for (int i = 0; i < e.first; i++) {
        ss << ' ';
    }
    ss << '^';
    for (int i = e.first + 1; i < e.last; i++) {
        ss << '~';
    }
    ss << '\n';
    ss << "expected: " << e.expected << '\n';
    ss << "got: '" << std::string(sql + e.first, sql + e.last) << '\'';
    return ss.str();
}

std::optional<sql_ast> parse_sql(const char* sql, bool success) {
    sql_parser parser;
    INFO("parsing '" << sql << "'");
    try {
        sql_ast ast = parser.parse(sql);
        if (!success) {
            FAIL("parse error expected\nast stmt length: " << ast.stmt.size());
        }
        return ast;
    }
    catch (const parse_error& e) {
        if (success) {
            FAIL(generate_error_message(e));
        }
        return std::nullopt;
    }
}

TEST_CASE("test sys_stmt", "[parser]") {
    using namespace Orange::parser;
    using boost::get;
    std::optional<sql_ast> ast;

    /* syntax */

    parse_sql("SHOW databases", false);
    parse_sql("a b c d", false);


    /* sys_stmt */

    parse_sql("showdatabases;", false);
    ast = parse_sql("show databases ;\n", true);
    ast = parse_sql("  show\ndatabases;", true);

    ast = parse_sql("show databases;", true);
    REQUIRE(ast.value().stmt.size() == 1);
    // show databases
    REQUIRE(ast.value().stmt[0].which() == (int)StmtKind::Sys);
    REQUIRE(get<sys_stmt>(ast.value().stmt[0]).which() == (int)SysStmtKind::ShowDb);


    /* db_stmt */

    ast = parse_sql("create database test1;drop database test2;", true);
    REQUIRE(ast.value().stmt.size() == 2);
    // create database
    REQUIRE(ast.value().stmt[0].which() == (int)StmtKind::Db);
    REQUIRE(get<db_stmt>(ast.value().stmt[0]).which() == (int)DbStmtKind::Create);
    REQUIRE(get<create_db_stmt>(get<db_stmt>(ast.value().stmt[0])).name == "test1");
    // drop database
    REQUIRE(ast.value().stmt[1].which() == (int)StmtKind::Db);
    REQUIRE(get<db_stmt>(ast.value().stmt[1]).which() == (int)DbStmtKind::Drop);
    REQUIRE(get<drop_db_stmt>(get<db_stmt>(ast.value().stmt[1])).name == "test2");


    /* tb_stmt */

    ast = parse_sql("create table aaa (col1 float not null, col2 varchar(2));", true);
    REQUIRE(ast.value().stmt.size() == 1);
    // create table
    REQUIRE(ast.value().stmt[0].which() == (int)StmtKind::Tb);
    REQUIRE(get<tb_stmt>(ast.value().stmt[0]).which() == (int)TbStmtKind::Create);
    REQUIRE(get<create_tb_stmt>(get<tb_stmt>(ast.value().stmt[0])).name == "aaa");
}
