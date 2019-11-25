#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4828)
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

BOOST_FUSION_ADAPT_STRUCT(Orange::sql_stmt, stmt_kind)

BOOST_FUSION_ADAPT_STRUCT(Orange::create_db_stmt, name)

BOOST_FUSION_ADAPT_STRUCT(Orange::create_tb_stmt, name, fields)


namespace Orange {
    // namespace
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    template <class Iterator>
    struct sql_grammer : qi::grammar<Iterator, sql_stmt_list(), qi::space_type> {
        template <class T>
        using rule = qi::rule<Iterator, T, qi::space_type>;

        rule<sql_stmt_list()> program;

        rule<sql_stmt()> stmt;

        rule<sql_stmt()> show_stmt;
        rule<show_db_stmt()> show_databases_stmt;
        rule<show_tb_stmt()> show_tables_stmt;

        rule<sql_stmt()> create_stmt;
        rule<create_db_stmt()> create_database_stmt;
        rule<create_tb_stmt()> create_table_stmt;
        rule<create_idx_stmt()> create_index_stmt;

        rule<sql_stmt()> drop_stmt;
        rule<sql_stmt()> use_stmt;
        rule<sql_stmt()> desc_stmt;
        rule<sql_stmt()> insert_stmt;
        rule<sql_stmt()> update_stmt;
        rule<sql_stmt()> select_stmt;
        rule<sql_stmt()> alter_stmt;

        rule<field_list()> fields;

        rule<std::string()> identifier;


        sql_grammer() : sql_grammer::base_type(program) {
            using namespace qi;

            // program := stmt*
            program %= skip(space)[*stmt];

            // stmt := ('SHOW' | 'CREATE' | 'DROP' | 'USE' | 'DESC' | 'INSERT' | 'UPDATE' |
            //          'SELECT' | 'ALTER') <REST> ';'
            stmt = (show_stmt | create_stmt | drop_stmt | use_stmt | desc_stmt | insert_stmt |
                    update_stmt | select_stmt | alter_stmt)[_val = _1] >>
                   ';';
            
            // show_stmt := 'SHOW' (show_databases_stmt | show_tables_stmt)
            show_stmt = no_case["SHOW"] >>
                        (show_databases_stmt[_val = _1] | show_tables_stmt[_val = _1]);
            show_databases_stmt = eps >> no_case["DATABASES"];  // sys_stmt
            show_tables_stmt = eps >> no_case["TABLES"];        // db_stmt

            // create_stmt := 'CREATE' (db_stmt | tb_stmt | idx_stmt)
            create_stmt =
                no_case["CREATE"] >> (create_database_stmt[_val = _1] |
                                      create_table_stmt[_val = _1] | create_index_stmt[_val = _1]);
            create_database_stmt %= omit[no_case["DATABASE"]] >> identifier;
            create_table_stmt %= omit[no_case["TABLE"]] >> identifier >> omit['('] >> fields >> omit[')'];
        }
    };

    SQL SqlParser::parse(const String& program) {
        auto iter = program.begin();
        auto end = program.end();
        auto parser = sql_grammer<String::const_iterator>();
        sql_stmt_list stmt_list;
        
        bool success = qi::phrase_parse(iter, end, parser, qi::space, stmt_list);
        if (success) {
            return SQL{stmt_list};
        } else {
            throw new ParseException(std::distance(iter, program.begin()));
        }
    }
}  // namespace Orange