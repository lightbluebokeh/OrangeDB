#include <catch/catch.hpp>
#include <iostream>
#include <optional>

#ifndef NDEBUG
#define BOOST_SPIRIT_DEBUG
#endif

#include <defs.h>
#include <orange/parser/parser.h>

using namespace Orange::parser;
std::optional<sql_ast> parse_sql(const char* sql) {
    sql_parser parser;
    INFO("parsing '" << sql << "'");
    try {
        sql_ast ast = parser.parse(sql);
        return ast;
    }
    catch (const parse_error& e) {
        std::cout << "FAILED: " << e.what() << " at " << e.first << '\n';
        std::cout << "    " << sql << '\n';
        std::cout << "    ";
        for (int i = 0; i < e.first; i++) {
            std::cout << ' ';
        }
        std::cout << '^';
        for (int i = e.first + 1; i < e.last; i++) {
            std::cout << '~';
        }
        std::cout << '\n';
        std::cout << "expected: " << e.expected << '\n';
        std::cout << "got: '" << std::string(sql + e.first, sql + e.last) << '\'' << std::endl;
        return std::nullopt;
    }
}

TEST_CASE("test parser", "[parser]") {
    using namespace Orange::parser;

    REQUIRE(!parse_sql("SHOW databases").has_value());
    REQUIRE(parse_sql("show databases;").has_value());
    REQUIRE(parse_sql("create database test;drop database test;").value().stmt.size() == 2);
    REQUIRE(parse_sql("create table aaa (col1 float not null, col2 varchar(2));").has_value());
}