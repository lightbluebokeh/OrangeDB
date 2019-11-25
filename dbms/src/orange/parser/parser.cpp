#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4828)
#pragma warning(disable : 26812)
#pragma warning(disable : 26495)
#endif

#include <boost/foreach.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant/recursive_variant.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifndef NDEBUG
#define BOOST_SPIRIT_DEBUG
#endif

#include <defs.h>

#include "parser.h"
#include "sql_ast.h"

// 以下是变魔术现场
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::sql_stmt, stmt_kind)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::create_db_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_db_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::use_db_stmt, name)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::create_tb_stmt, name, fields)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_tb_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::desc_tb_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::insert_into_tb_stmt, name, values)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::delete_from_tb_stmt, name, where_clause)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::update_tb_stmt, name, set, where)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::select_tb_stmt, select, tables, where)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::create_idx_stmt, idx_name, tb_name, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_idx_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::alter_add_idx_stmt, tb_name, idx_name, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::alter_drop_idx_stmt, tb_name, idx_name)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::alter_stmt, tb_name, alter_stmt_kind)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_field_stmt, new_field)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_col_stmt, col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::change_col_stmt, col_name, new_field)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::rename_tb_stmt, new_tb_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_primary_key_stmt, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_constraint_primary_key_stmt, pk_name, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_primary_key_stmt, pk_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_constraint_foreign_key_stmt, fk_name, col_list,
                          ref_tb_name, ref_col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_foreign_key_stmt, fk_name)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::column, table_name, col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::data_type, value)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_def, col_name, type, is_null, default_value)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_primary_key, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_foreign_key, col, ref_table_name, ref_col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::where_clause, col_name, cond)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::set_clause, col_name)

// 表演完成


namespace Orange {
    namespace parser {
        // namespace
        namespace qi = boost::spirit::qi;
        namespace ascii = boost::spirit::ascii;

        template <class Iterator>
        struct sql_grammer : qi::grammar<Iterator, sql_stmt_list(), qi::space_type> {
            template <class T>
            using rule = qi::rule<Iterator, T, qi::space_type>;

            // program
            rule<sql_stmt_list()> program;

            // stmt
            rule<sql_stmt()> stmt;

            rule<sql_stmt()> show_stmt;
            rule<show_db_stmt()> show_databases_stmt;
            rule<show_tb_stmt()> show_tables_stmt;

            rule<sql_stmt()> create_stmt;
            rule<create_db_stmt()> create_database_stmt;
            rule<create_tb_stmt()> create_table_stmt;
            rule<create_idx_stmt()> create_index_stmt;

            rule<sql_stmt()> drop_stmt;
            rule<drop_db_stmt()> drop_database_stmt;
            rule<drop_tb_stmt()> drop_table_stmt;
            rule<drop_idx_stmt()> drop_index_stmt;

            rule<use_db_stmt()> use_database_stmt;

            rule<desc_tb_stmt()> desc_stmt;
            rule<insert_into_tb_stmt()> insert_into_table_stmt;
            rule<delete_from_tb_stmt()> delete_from_table_stmt;
            rule<update_tb_stmt()> update_table_stmt;
            rule<select_tb_stmt()> select_from_table_stmt;

            rule<sql_stmt()> alter_stmt;
            rule<sql_stmt()> alter_add;
            rule<sql_stmt()> alter_drop;
            rule<change_col_stmt()> alter_change;
            rule<rename_tb_stmt()> alter_rename;

            rule<column()> col;
            rule<column_list()> col_list;

            rule<selector()> select;

            rule<op()> oprt;

            rule<data_type()> type;

            rule<value()> val;
            rule<value_list()> val_list;
            rule<value_lists()> val_lists;

            rule<expr()> expression;

            rule<table_list()> tables;

            rule<field_list()> fields;

            rule<std::string()> identifier;

            // 大型语法解析现场
            sql_grammer() : sql_grammer::base_type(program) {
                using namespace qi;

                // program := stmt*
                program %= skip(space)[*stmt];

                // stmt := ('SHOW' | 'CREATE' | 'DROP' | 'USE' | 'DESC' | 'INSERT' | 'DELETE' |
                //          'UPDATE' | 'SELECT' | 'ALTER') <REST> ';'  (左公因子)
                stmt = (show_stmt[_val = _1] | create_stmt[_val = _1] | drop_stmt[_val = _1] |
                        use_database_stmt[_val = _1] | desc_stmt[_val = _1] |
                        insert_into_table_stmt[_val = _1] | delete_from_table_stmt[_val = _1] |
                        update_table_stmt[_val = _1] | select_from_table_stmt[_val = _1] |
                        alter_stmt[_val = _1]) >>
                       ';';  // clang-format喜欢把这个放下面

                // show_stmt := 'SHOW' (show_databases | show_tables)
                show_stmt = no_case["SHOW"] >>
                            (show_databases_stmt[_val = _1] | show_tables_stmt[_val = _1]);
                show_databases_stmt = eps >> no_case["DATABASES"];  // sys_stmt
                show_tables_stmt = eps >> no_case["TABLES"];        // db_stmt

                // create_stmt := 'CREATE' (create_database | create_table | idx_stmt)
                create_stmt = no_case["CREATE"] >>
                              (create_database_stmt[_val = _1] | create_table_stmt[_val = _1] |
                               create_index_stmt[_val = _1]);
                create_database_stmt %= omit[no_case["DATABASE"]] >> identifier;  // db_stmt
                create_table_stmt %=
                    omit[no_case["TABLE"]] >> identifier >> omit['('] >> fields >> omit[')'];
                create_index_stmt %= omit[no_case["INDEX"]] >> identifier >> omit[no_case["ON"]] >>
                                     identifier >> omit['('] >> col_list >> omit[')'];

            }
        };

        SqlAst SqlParser::parse(const String& program) {
            auto iter = program.begin();
            auto end = program.end();
            auto parser = sql_grammer<String::const_iterator>();
            sql_stmt_list stmts;

            bool success = qi::phrase_parse(iter, end, parser, qi::space, stmts);
            if (success) {
                return SqlAst(std::move(stmts));
            } else {
                throw new ParseException(std::distance(iter, program.begin()));
            }
        }
    }  // namespace parser
}  // namespace Orange