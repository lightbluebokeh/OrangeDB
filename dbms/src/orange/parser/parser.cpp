// parser

#if !defined(NDEBUG) && defined(DEBUG_PARSER)
#define BOOST_SPIRIT_DEBUG
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4828)
#pragma warning(disable : 26812)
#pragma warning(disable : 26495)
#endif

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant/recursive_variant.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <defs.h>

#include "parser.h"
#include "sql_ast.h"

// 以下是变魔术现场
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::sql_ast, stmt)

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

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_field_stmt, table_name, new_field)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_col_stmt, table_name, col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::change_col_stmt, table_name, col_name, new_field)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::rename_tb_stmt, table_name, new_tb_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_primary_key_stmt, table_name, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_constraint_primary_key_stmt, table_name, pk_name,
                          col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_primary_key_stmt, table_name, pk_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::add_constraint_foreign_key_stmt, table_name, fk_name,
                          col_list, ref_tb_name, ref_col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_foreign_key_stmt, table_name, fk_name)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::column, table_name, col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::data_type, type, value)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_def, col_name, type, is_null, default_value)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_primary_key, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_foreign_key, col, ref_table_name, ref_col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_where_op, col_name, operator_, expression)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_where_null, col_name, is_null)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_set, col_name)

// 表演完成


namespace Orange {
    namespace parser {
        // namespace
        namespace qi = boost::spirit::qi;
        namespace ascii = boost::spirit::ascii;
        namespace phoenix = boost::phoenix;

        template <class Iterator>
        struct sql_grammer : qi::grammar<Iterator, sql_ast(), qi::space_type> {
            template <class T>
            using rule = qi::rule<Iterator, T, qi::space_type>;

            // program
            rule<sql_ast()> program;

            // stmt
            rule<sql_stmt()> stmt;

            rule<sys_stmt()> sys;
            rule<show_db_stmt()> show_db;

            rule<db_stmt()> db;
            rule<show_tb_stmt()> show_tb;
            rule<create_db_stmt()> create_db;
            rule<drop_db_stmt()> drop_db;
            rule<use_db_stmt()> use_db;

            rule<tb_stmt()> tb;
            rule<create_tb_stmt()> create_tb;
            rule<drop_tb_stmt()> drop_tb;
            rule<desc_tb_stmt()> desc_tb;
            rule<insert_into_tb_stmt()> insert_into_tb;
            rule<delete_from_tb_stmt()> delete_from_tb;
            rule<update_tb_stmt()> update_tb;
            rule<select_tb_stmt()> select_tb;

            rule<idx_stmt()> idx;
            rule<create_idx_stmt()> create_idx;
            rule<drop_idx_stmt()> drop_idx;
            rule<alter_add_idx_stmt()> alter_add_idx;
            rule<alter_drop_idx_stmt()> alter_drop_idx;

            rule<alter_stmt()> alter;
            rule<add_field_stmt()> add_field;
            rule<drop_col_stmt()> drop_col;
            rule<change_col_stmt()> change_col;
            rule<rename_tb_stmt()> rename_tb;
            rule<add_primary_key_stmt()> add_primary_key;
            rule<add_constraint_primary_key_stmt()> add_constraint_primary_key;
            rule<drop_primary_key_stmt()> drop_primary_key;
            rule<add_constraint_foreign_key_stmt()> add_constraint_foreign_key;
            rule<drop_foreign_key_stmt()> drop_foreign_key;

            // others
            rule<std::string()> identifier;
            rule<std::string()> value_string;

            rule<column()> col;

            rule<column_list()> columns;
            rule<table_list()> tables;

            rule<selector()> selectors;

            rule<op()> operator_;

            rule<data_type()> type;

            rule<data_value()> value;
            rule<data_value_list()> value_list;
            rule<data_value_lists()> value_lists;

            rule<expr()> expression;

            rule<single_where()> where;
            rule<single_where_op()> where_op;
            rule<single_where_null()> where_null;
            rule<where_clause()> where_list;

            rule<single_set()> set;
            rule<set_clause()> set_list;

            rule<single_field()> field;
            rule<field_def()> definition_field;
            rule<field_primary_key()> primary_key_field;
            rule<field_foreign_key()> foreign_key_field;
            rule<field_list()> fields;

            sql_grammer() : sql_grammer::base_type(program) {
                using namespace qi;
                using phoenix::at_c;
                using phoenix::bind;
                using phoenix::construct;
                using phoenix::val;

                // <program> := <stmt>*
                program %= *stmt >> eps;

                // <stmt> := (<sys_stmt> | <db_stmt> | <tb_stmt> | <idx_stmt> | <alter_stmt>) ';'
                stmt %= (sys | db | tb | idx | alter) > ';';

                // <sys_stmt> := <show_db>
                sys %= show_db >> eps;
                // <show_db> := 'SHOW' 'DATABASES'
                show_db = no_case["SHOW"] >> no_case["DATABASES"];

                // <db_stmt> := <show_tb> | <create_db> | <drop_db> | <use_db>
                db %= show_tb | create_db | drop_db | use_db;
                // <show_tb> := 'SHOW' 'TABLES'
                show_tb = no_case["CREATE"] >> no_case["TABLES"];
                // <create_db> := 'CREATE' 'DATABASE' [db_name]
                create_db %= no_case["CREATE"] >> no_case["DATABASE"] > identifier;
                // <drop_db> := 'DROP' 'DATABASE' [db_name]
                drop_db %= no_case["DROP"] >> no_case["DATABASE"] > identifier;
                // <use_db> := 'USE' [db_name]
                use_db %= no_case["USE"] > identifier;

                // <tb_stmt> := <create_tb> | <drop_tb> | <desc_tb> | <insert_into_tb> |
                //              <delete_from_tb> | <update_tb> | <select_tb>
                tb %= create_tb | drop_tb | desc_tb | insert_into_tb | delete_from_tb | update_tb |
                      select_tb;
                // <create_tb> := 'CREATE' 'TABLE' [tb_name] '(' <field_list> ')'
                create_tb %=
                    no_case["CREATE"] >> no_case["TABLE"] > identifier > '(' > fields > ')';
                // <drop_tb> := 'DROP' 'TABLE' [tb_name]
                drop_tb %= no_case["DROP"] >> no_case["TABLE"] > identifier;
                // <desc_tb> := 'DESC' [tb_name]
                desc_tb %= no_case["DESC"] > identifier;
                // <insert_into_tb> := 'INSERT' 'INTO' [tb_name] 'VALUES' <value_lists>
                insert_into_tb %= no_case["INSERT"] > no_case["INTO"] > identifier >
                                  no_case["VALUES"] > value_lists;
                // <delete_from_tb> := 'DELETE' 'FROM' [tb_name] 'WHERE' <where_clause>
                delete_from_tb %= no_case["DELETE"] > no_case["FROM"] > identifier >
                                  no_case["WHERE"] > where_list;
                // <update_tb> := 'UPDATE' [tb_name] 'SET' <set_clause> 'WHERE' <where_clause>
                update_tb %= no_case["UPDATE"] > identifier > no_case["SET"] > set_list >
                             no_case["WHERE"] > where_list;
                // <select_tb> := 'SELECT' <selector> 'FROM' <table_list> 'WHERE' <where_clause>
                select_tb %= no_case["SELECT"] > selectors > no_case["FROM"] > tables >
                             no_case["WHERE"] > where_list;

                // <idx_stmt> := <create_idx> | <drop_idx> | <alter_add_idx> | <alter_drop_idx>
                idx %= create_idx | drop_idx | alter_add_idx | alter_drop_idx;
                // <create_idx> := 'CREATE' 'INDEX' [idx_name] 'ON' [tb_name] '(' <column_list> ')'
                create_idx %= no_case["CREATE"] >> no_case["INDEX"] > identifier > no_case["ON"] >
                              identifier > '(' > columns > ')';
                // <drop_idx> := 'DROP' 'INDEX' [idx_name]
                drop_idx %= no_case["DROP"] >> no_case["INDEX"] > identifier;
                // <alter_add_idx> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'INDEX' [idx_name]
                //                    '(' column_list ')'
                alter_add_idx %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                                 no_case["ADD"] >> no_case["INDEX"] > identifier > '(' > columns >
                                 ')';
                // <alter_drop_idx> := 'ALTER' 'TABLE' [tb_name] 'DROP' 'INDEX' [idx_name]
                alter_drop_idx %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                                  no_case["DROP"] >> no_case["INDEX"] > identifier;

                // <alter_stmt> := <add_field> | <drop_col> | <change_col> | <rename_tb> |
                //                 <add_primary_key> | <add_constraint_primary_key> |
                //                 <drop_primary_key> | <add_constraint_foreign_key> |
                //                 <drop_foreign_key>
                alter %= add_field | drop_col | change_col | rename_tb | add_primary_key |
                         add_constraint_primary_key | drop_primary_key |
                         add_constraint_foreign_key | drop_foreign_key;
                // <add_field> := 'ALTER' 'TABLE' [tb_name] 'ADD' <field>
                add_field %=
                    no_case["ALTER"] > no_case["TABLE"] > identifier >> no_case["ADD"] >> field;
                // <drop_col> := 'ALTER' 'TABLE' [tb_name] 'DROP' [col_name]
                drop_col %= no_case["ALTER"] > no_case["TABLE"] > identifier >> no_case["DROP"] >>
                            identifier;
                // <change_col> := 'ALTER' 'TABLE' [tb_name] 'CHANGE' [col_name] <field>
                change_col %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                              no_case["CHANGE"] > identifier > field;
                // <rename_tb> := 'ALTER' 'TABLE' [tb_name] 'RENAME' 'TO' [new_tb_name]
                change_col %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                              no_case["RENAME"] > no_case["TO"] > identifier;
                // <add_primary_key> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'PRIMARY' 'KEY'
                //                      '(' <column_list> ')'
                add_primary_key %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                                   no_case["ADD"] >> no_case["PRIMARY"] > no_case["KEY"] > '(' >
                                   columns > ')';
                // <add_constraint_primary_key> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'CONSTRAINT'
                //                                 [pk_name] 'PRIMARY' 'KEY' '(' <column_list> ')'
                add_constraint_primary_key %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                                              no_case["ADD"] >> no_case["CONSTRAINT"] >
                                              identifier >> no_case["PRIMARY"] > no_case["KEY"] >
                                              '(' > columns > ')';
                // <drop_primary_key> := 'ALTER' 'TABLE' [tb_name] 'DROP' 'PRIMARY' 'KEY' [pk_name]?
                drop_primary_key %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                                    no_case["DROP"] >> no_case["PRIMARY"] > no_case["KEY"] >
                                    -(identifier);
                // <add_constraint_foreign_key> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'CONSTRAINT'
                //                                 [fk_name] 'FOREIGN' 'KEY' '(' <column_list> ')'
                //                                 'REFERENCES' [ref_tb_name] '(' <column_list> ')'
                add_constraint_foreign_key %=
                    no_case["ALTER"] > no_case["TABLE"] > identifier >> no_case["ADD"] >>
                    no_case["CONSTRAINT"] > identifier >> no_case["FOREIGN"] > no_case["KEY"] >
                    '(' > columns > ')' > no_case["REFERENCES"] > identifier > '(' > columns > ')';
                // <drop_foreign_key> := 'ALTER' 'TABLE' [tb_name] 'DROP' 'FOREIGN' 'KEY' [fk_name]
                drop_foreign_key %= no_case["ALTER"] > no_case["TABLE"] > identifier >>
                                    no_case["DROP"] >> no_case["FOREIGN"] > no_case["KEY"] >
                                    identifier;

                // <identifier> := [A-Za-z][_0-9A-Za-z]*
                identifier %= lexeme[char_("A-Za-z") > *char_("_0-9A-Za-z")];

                // <string> := "'" [^']* "'"
                value_string %= '\'' > lexeme[*((char_ - '\'') | "\\'")] > '\'';

                // <column> := ([tb_name] '.')? [col_name]
                col %= -(identifier >> '.') > identifier;

                // <column_list> := [col_name] (',' [col_name])*
                columns %= identifier % ',';

                // <table_list> := [tb_name] (',' [tb_name])*
                tables %= identifier % ',';

                // <selector> := '*' | <col> (',' <col>)*
                selectors %= '*' | (col % ',');

                // <op> := '=' | '<>' | '<=' | '>=' | '<' | '>'
                operator_ = char_("=")[_val = op::Eq] | char_("<>")[_val = op::Neq] |
                            char_("<=")[_val = op::Le] | char_(">=")[_val = op::Ge] |
                            char_("<")[_val = op::Ls] | char_(">")[_val = op::Gt];

                // <type> := ('INT' '(' <int> ')') | ('VARCHAR' '(' <int> ')') | 'DATE' | 'FLOAT'
                type = (no_case["INT"] > '(' >
                        int_[at_c<0>(_val) = DataTypeKind::Int, at_c<1>(_val) = _1] > ')') |
                       (no_case["VARCHAR"] > '(' >
                        int_[at_c<0>(_val) = DataTypeKind::VarChar, at_c<1>(_val) = _1] > ')') |
                       no_case["DATE"][at_c<0>(_val) = DataTypeKind::Date] |
                       no_case["FLOAT"][at_c<0>(_val) = DataTypeKind::Float];

                // <value> := <int> | <string> | 'NULL'
                value %= int_ | value_string | no_case["NULL"];

                // <value_list> := <value> (',' <value>)*
                value_list %= value % ',';

                // <value_lists> := '(' <value_list> ')' (',' '(' <value_list> ')')*
                value_lists %= ('(' > value_list > ')') % ',';

                // <expr> := <value> | <col>
                expression %= value | col;

                // <where> := <where_op> | <where_null>
                where %= where_op | where_null;
                // <where_op> := <col> <op> <expr>
                where_op %= identifier > operator_ > expression;
                // <where_null> := <col> 'IS' 'NOT'? 'NULL'
                where_null %=
                    identifier > no_case["IS"] > matches[!no_case["NOT"]] > no_case["NULL"];

                // <where_clause> := <where> ('AND' <where>)*
                where_list %= where % ',';

                // <set> := [col_name] '=' <value>
                set %= identifier > '=' > value;

                // <set_clause> := <set> (',' <set>)*
                set_list %= set % ',';

                // <field> := field_def | field_primary_key | field_foreign_key
                field %= definition_field | primary_key_field | foreign_key_field;
                // <field_def> := [col_name] <type> ('NOT' 'NULL')? ('DEFAULT' <value>)?
                definition_field %= identifier > type > matches[no_case["NOT"] > no_case["NULL"]] >
                                    -(no_case["DEFAULT"] > value);
                // <field_primary_key> := 'PRIMARY' 'KEY' '(' <column_list> ')'
                primary_key_field %= no_case["PRIMARY"] > no_case["KEY"] > '(' > columns > ')';
                // <field_foreign_key> := 'FOREIGN' 'KEY' '(' [col_name] ')' 'REFERENCES' [tb_name]
                //                        '(' [col_name] ')'
                foreign_key_field %= no_case["PRIMARY"] > no_case["KEY"] > '(' > identifier > ')' >
                                     no_case["REFERENCES"] > identifier > '(' > identifier > ')';

                BOOST_SPIRIT_DEBUG_NODES((program)(stmt));
                BOOST_SPIRIT_DEBUG_NODES((sys)(show_db));
                BOOST_SPIRIT_DEBUG_NODES((db)(show_tb)(create_db)(drop_db)(use_db));
                BOOST_SPIRIT_DEBUG_NODES((tb)(create_tb)(drop_tb)(desc_tb)(insert_into_tb)(
                    delete_from_tb)(update_tb)(select_tb));
                BOOST_SPIRIT_DEBUG_NODES(
                    (idx)(create_idx)(drop_idx)(alter_add_idx)(alter_drop_idx));
                BOOST_SPIRIT_DEBUG_NODES((alter)(add_field)(drop_col)(change_col)(rename_tb)(
                    add_primary_key)(add_constraint_primary_key)(drop_primary_key)(
                    add_constraint_foreign_key)(drop_foreign_key));
                BOOST_SPIRIT_DEBUG_NODES((identifier)(value_string));
                BOOST_SPIRIT_DEBUG_NODES((col)(columns)(tables)(selectors));
                BOOST_SPIRIT_DEBUG_NODES(
                    (operator_)(type)(value)(value_list)(value_lists)(expression));
                BOOST_SPIRIT_DEBUG_NODES((where)(where_op)(where_null)(where_list));
                BOOST_SPIRIT_DEBUG_NODES((set)(set_list));
                BOOST_SPIRIT_DEBUG_NODES(
                    (field)(definition_field)(primary_key_field)(foreign_key_field)(fields));
            }
        };

        sql_ast sql_parser::parse(const String& sql) {
            auto iter = sql.begin();
            auto end = sql.end();
            auto parser = sql_grammer<String::const_iterator>();
            sql_ast ast;

            bool success;
            try {
                success = qi::phrase_parse(iter, end, parser, qi::space, ast);
            }
            catch (const qi::expectation_failure<String::const_iterator>& e) {
                throw parse_error("parse error", std::distance(sql.begin(), e.first),
                                  std::distance(sql.begin(), e.last), e.what_.tag);
            }
            if (success && iter == end) {
                return ast;
            } else {
                throw parse_error("unexpected statement", std::distance(sql.begin(), iter),
                                  std::distance(sql.begin(), iter), "");
            }
        }
    }  // namespace parser
}  // namespace Orange