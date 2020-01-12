
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
#include <boost/spirit/include/support_standard.hpp>
#include <boost/spirit/include/support_standard_wide.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <clocale>
#include <defs.h>

#include "parser.h"
#include "sql_ast.h"

// 以下是变魔术现场
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::sql_ast, stmt_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::sql_stmt, stmt)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::sys_stmt, stmt)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::db_stmt, stmt)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::create_db_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_db_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::use_db_stmt, name)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::tb_stmt, stmt)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::create_tb_stmt, name, fields)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_tb_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::desc_tb_stmt, name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::insert_into_tb_stmt, name, columns, values_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::delete_from_tb_stmt, name, where)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::update_tb_stmt, name, set, where)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::select_tb_stmt, select, tables, where, group_by, limit)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::idx_stmt, stmt)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::create_idx_stmt, idx_name, tb_name, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::drop_idx_stmt, idx_name, tb_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::alter_add_idx_stmt, tb_name, idx_name, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::alter_drop_idx_stmt, tb_name, idx_name)

BOOST_FUSION_ADAPT_STRUCT(Orange::parser::alter_stmt, stmt)
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
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::data_value, value)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::data_type, kind, value, value2)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::expr, expression)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_def, col_name, type, is_not_null, default_value)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_primary_key, col_list)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::field_foreign_key, col, ref_table_name, ref_col_name)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_field, field)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_where_op, col, operator_, expression)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_where_null, col, is_not_null)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_where_like, col, pattern)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_where, where)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::single_set, col_name, val)
BOOST_FUSION_ADAPT_STRUCT(Orange::parser::aggregate_func_selector, func, col)


namespace Orange::parser {
    // using some namespace
    namespace qi = boost::spirit::qi;
    namespace phoenix = boost::phoenix;
    // parser encoding
    namespace encoding = boost::spirit::standard;

    template <class Iterator, class Skipper = encoding::space_type>
    struct sql_grammer : qi::grammar<Iterator, sql_ast(), Skipper> {
        template <class T>
        using rule = qi::rule<Iterator, T, Skipper>;

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
        rule<qi::unused_type(const char*)> kw;
        rule<std::string()> identifier;
        rule<std::string()> value_string;

        rule<column()> col;

        rule<column_list()> columns;
        rule<table_list()> tables;

        rule<aggregate_func_selector()> aggr_func;
        rule<selector()> selectors;

        rule<op()> operator_;

        rule<data_type()> type;

        rule<data_value()> value;
        rule<data_values()> values;
        rule<data_values_list()> values_list;

        rule<expr()> expression;

        rule<single_where()> where;
        rule<single_where_op()> where_op;
        rule<single_where_null()> where_null;
        rule<single_where_like()> where_like;
        rule<where_clause()> where_list;

        rule<single_set()> set;
        rule<set_clause()> set_list;

        rule<single_field()> field;
        rule<field_def()> definition_field;
        rule<field_primary_key()> primary_key_field;
        rule<field_foreign_key()> foreign_key_field;
        rule<field_list()> fields;

        explicit sql_grammer() : sql_grammer::base_type(program) {
            using phoenix::at_c;
            using phoenix::bind;
            using phoenix::construct;
            using phoenix::push_back;
            using phoenix::val;

            // <program> := <stmt>*
            program = *(stmt[push_back(at_c<0>(qi::_val), qi::_1)] | ';') > qi::eoi;

            // <stmt> := (<sys_stmt> | <db_stmt> | <tb_stmt> | <idx_stmt> | <alter_stmt>) ';'
            stmt %= (sys | db | tb | idx | alter) > ';';

            // <sys_stmt> := <show_db>
            sys %= qi::eps >> show_db;
            // <show_db> := 'SHOW' 'DATABASES'
            show_db = kw(+"SHOW") >> kw(+"DATABASES");

            // <db_stmt> := <show_tb> | <create_db> | <drop_db> | <use_db>
            db %= show_tb | create_db | drop_db | use_db;
            // <show_tb> := 'SHOW' 'TABLES'
            show_tb = kw(+"SHOW") >> kw(+"TABLES");
            // <create_db> := 'CREATE' 'DATABASE' [db_name]
            create_db %= kw(+"CREATE") >> kw(+"DATABASE") > identifier;
            // <drop_db> := 'DROP' 'DATABASE' [db_name]
            drop_db %= kw(+"DROP") >> kw(+"DATABASE") > identifier;
            // <use_db> := 'USE' [db_name]
            use_db %= kw(+"USE") > identifier;

            // <tb_stmt> := <create_tb> | <drop_tb> | <desc_tb> | <insert_into_tb> |
            //              <delete_from_tb> | <update_tb> | <select_tb>
            tb %= create_tb | drop_tb | desc_tb | insert_into_tb | delete_from_tb | update_tb |
                  select_tb;
            // <create_tb> := 'CREATE' 'TABLE' [tb_name] '(' <field_list> ')'
            create_tb %= kw(+"CREATE") >> kw(+"TABLE") > identifier > '(' > fields > ')';
            // <drop_tb> := 'DROP' 'TABLE' [tb_name]
            drop_tb %= kw(+"DROP") >> kw(+"TABLE") > identifier;
            // <desc_tb> := 'DESC' [tb_name]
            desc_tb %= kw(+"DESC") > identifier;
            // <insert_into_tb> := 'INSERT' 'INTO' [tb_name] ('(' <column_list> ')')?
            //                     'VALUES' '(' <values> ')'
            insert_into_tb %= kw(+"INSERT") > kw(+"INTO") > identifier > -('(' > columns > ')') >
                              kw(+"VALUES") > values_list;
            // <delete_from_tb> := 'DELETE' 'FROM' [tb_name] 'WHERE' <where_clause>
            delete_from_tb %= kw(+"DELETE") > kw(+"FROM") > identifier > kw(+"WHERE") > where_list;
            // <update_tb> := 'UPDATE' [tb_name] 'SET' <set_clause> 'WHERE' <where_clause>
            update_tb %=
                kw(+"UPDATE") > identifier > kw(+"SET") > set_list > kw(+"WHERE") > where_list;
            // <select_tb> := 'SELECT' <selector> 'FROM' <table_list> ('WHERE' <where_clause>)?
            //                ('GROUP' 'BY' <col>) ('LIMIT' <int>)
            select_tb = kw(+"SELECT") > selectors[at_c<0>(qi::_val) = qi::_1] > kw(+"FROM") >
                        tables[at_c<1>(qi::_val) = qi::_1] >
                        -(kw(+"WHERE") > where_list)[at_c<2>(qi::_val) = qi::_1] >
                        -(kw(+"GROUP") > kw(+"BY") > col[at_c<3>(qi::_val) = qi::_1]) >
                        -(kw(+"LIMIT") > qi::int_)[at_c<4>(qi::_val) = qi::_1];

            // <idx_stmt> := <create_idx> | <drop_idx> | <alter_add_idx> | <alter_drop_idx>
            idx %= create_idx | drop_idx | alter_add_idx | alter_drop_idx;
            // <create_idx> := 'CREATE' 'INDEX' [idx_name] 'ON' [tb_name] '(' <column_list> ')'
            create_idx %= kw(+"CREATE") >> kw(+"INDEX") > identifier > kw(+"ON") > identifier >
                          '(' > columns > ')';
            // <drop_idx> := 'DROP' 'INDEX' [idx_name] 'ON' [tb_name]
            drop_idx %= kw(+"DROP") >> kw(+"INDEX") > identifier > kw(+"ON") > identifier;
            // <alter_add_idx> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'INDEX' [idx_name]
            //                    '(' column_list ')'
            alter_add_idx %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"ADD") >>
                             kw(+"INDEX") > identifier > '(' > columns > ')';
            // <alter_drop_idx> := 'ALTER' 'TABLE' [tb_name] 'DROP' 'INDEX' [idx_name]
            alter_drop_idx %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"DROP") >>
                              kw(+"INDEX") > identifier;

            // <alter_stmt> := <add_field> | <drop_col> | <change_col> | <rename_tb> |
            //                 <add_primary_key> | <add_constraint_primary_key> |
            //                 <drop_primary_key> | <add_constraint_foreign_key> |
            //                 <drop_foreign_key>
            alter %= drop_col | change_col | rename_tb | add_primary_key |
                     add_constraint_primary_key | drop_primary_key | add_constraint_foreign_key |
                     drop_foreign_key | add_field;
            // <add_field> := 'ALTER' 'TABLE' [tb_name] 'ADD' <field>
            add_field %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"ADD") >> field;
            // <drop_col> := 'ALTER' 'TABLE' [tb_name] 'DROP' [col_name]
            drop_col %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"DROP") >> identifier;
            // <change_col> := 'ALTER' 'TABLE' [tb_name] 'CHANGE' [col_name] <field>
            change_col %=
                kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"CHANGE") > identifier > field;
            // <rename_tb> := 'ALTER' 'TABLE' [tb_name] 'RENAME' 'TO' [new_tb_name]
            rename_tb %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"RENAME") > kw(+"TO") >
                         identifier;
            // <add_primary_key> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'PRIMARY' 'KEY'
            //                      '(' <column_list> ')'
            add_primary_key %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"ADD") >>
                               kw(+"PRIMARY") >> kw(+"KEY") > '(' > columns > ')';
            // <add_constraint_primary_key> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'CONSTRAINT'
            //                                 [pk_name] 'PRIMARY' 'KEY' '(' <column_list> ')'
            add_constraint_primary_key %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >>
                                          kw(+"ADD") >> kw(+"CONSTRAINT") >> identifier >>
                                          kw(+"PRIMARY") >> kw(+"KEY") >> '(' >> columns >> ')';
            // <drop_primary_key> := 'ALTER' 'TABLE' [tb_name] 'DROP' 'PRIMARY' 'KEY' [pk_name]?
            drop_primary_key %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"DROP") >>
                                kw(+"PRIMARY") >> kw(+"KEY") > -(identifier);
            // <add_constraint_foreign_key> := 'ALTER' 'TABLE' [tb_name] 'ADD' 'CONSTRAINT'
            //                                 [fk_name] 'FOREIGN' 'KEY' '(' <column_list> ')'
            //                                 'REFERENCES' [ref_tb_name] '(' <column_list> ')'
            add_constraint_foreign_key %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >>
                                          kw(+"ADD") >> kw(+"CONSTRAINT") > identifier >>
                                          kw(+"FOREIGN") > kw(+"KEY") > '(' > columns > ')' >
                                          kw(+"REFERENCES") > identifier > '(' > columns > ')';
            // <drop_foreign_key> := 'ALTER' 'TABLE' [tb_name] 'DROP' 'FOREIGN' 'KEY' [fk_name]
            drop_foreign_key %= kw(+"ALTER") >> kw(+"TABLE") >> identifier >> kw(+"DROP") >>
                                kw(+"FOREIGN") > kw(+"KEY") > identifier;

            // keyword
            kw = qi::no_case[qi::lit(qi::_r1)] >> qi::no_skip[&!(encoding::alnum | '_')];

            // <identifier> := [a-zA-Z][_0-9a-zA-Z]*
            identifier %=
                !(kw(+"DATABASE") | kw(+"DATABASES") | kw(+"TABLE") | kw(+"TABLES") | kw(+"SHOW") |
                  kw(+"CREATE") | kw(+"DROP") | kw(+"USE") | kw(+"PRIMARY") | kw(+"KEY") |
                  kw(+"NOT") | kw(+"NULL") | kw(+"INSERT") | kw(+"INTO") | kw(+"VALUES") |
                  kw(+"DELETE") | kw(+"FROM") | kw(+"WHERE") | kw(+"UPDATE") | kw(+"SET") |
                  kw(+"SELECT") | kw(+"IS") | kw(+"INT") | kw(+"VARCHAR") | kw(+"DEFAULT") |
                  kw(+"CONSTRAINT") | kw(+"CHANGE") | kw(+"ALTER") | kw(+"ADD") | kw(+"RENAME") |
                  kw(+"DESC") | kw(+"INDEX") | kw(+"AND") | kw(+"DATE") | kw(+"FLOAT") |
                  kw(+"FOREIGN") | kw(+"REFERENCES")) >>
                qi::lexeme[encoding::char_("a-zA-Z") > *(encoding::char_("_0-9a-zA-Z") | '_')];

            // <string> := "'" [^']* "'"
            value_string %= '\'' > qi::no_skip[*(~encoding::char_('\'') | "\\'")] > '\'';

            // <column> := ([tb_name] '.')? [col_name]
            col = -(identifier >> '.')[at_c<0>(qi::_val) = qi::_1] >
                  identifier[at_c<1>(qi::_val) = qi::_1];

            // <column_list> := [col_name] (',' [col_name])*
            columns %= identifier % ',';

            // <table_list> := [tb_name] (',' [tb_name])*
            tables %= identifier % ',';

            // <selector> := '*' | (col | <aggr_func>) (',' <col>)*
            selectors %= '*' | ((aggr_func | col) % ',');
            // <aggr_func> := [FUNC_NAME] '(' <col> ')'
            aggr_func = (kw(+"Avg")[at_c<0>(qi::_val) = aggregate_func::Avg] >> '(' >
                         col[at_c<1>(qi::_val) = qi::_1] > ')') |
                        (kw(+"COUNT")[at_c<0>(qi::_val) = aggregate_func::Count] >> '(' >
                         col[at_c<1>(qi::_val) = qi::_1] > ')') |
                        (kw(+"MAX")[at_c<0>(qi::_val) = aggregate_func::Max] >> '(' >
                         col[at_c<1>(qi::_val) = qi::_1] > ')') |
                        (kw(+"MIN")[at_c<0>(qi::_val) = aggregate_func::Min] >> '(' >
                         col[at_c<1>(qi::_val) = qi::_1] > ')') |
                        (kw(+"SUM")[at_c<0>(qi::_val) = aggregate_func::Sum] >> '(' >
                         col[at_c<1>(qi::_val) = qi::_1] > ')') |
                        (kw(+"VAR")[at_c<0>(qi::_val) = aggregate_func::Var] >> '(' >
                         col[at_c<1>(qi::_val) = qi::_1] > ')');

            // <op> := '=' | '<>' | '<=' | '>=' | '<' | '>'
            operator_ = qi::lit("=")[qi::_val = op::Eq] | qi::lit("<>")[qi::_val = op::Neq] |
                        qi::lit("<=")[qi::_val = op::Le] | qi::lit(">=")[qi::_val = op::Ge] |
                        qi::lit("<")[qi::_val = op::Lt] | qi::lit(">")[qi::_val = op::Gt];

            // <type> := ('INT' ('(' <int> ')')?) | ('VARCHAR' '(' <int> ')') | 'DATE' | 'FLOAT'
            type =
                (kw(+"INT") >
                 -('(' > qi::int_[at_c<0>(qi::_val) = orange_t::Int, at_c<1>(qi::_val) = qi::_1] >
                   ')')) |
                (kw(+"CHAR") > '(' >
                 qi::int_[at_c<0>(qi::_val) = orange_t::Char, at_c<1>(qi::_val) = qi::_1] > ')') |
                (kw(+"VARCHAR") > '(' >
                 qi::int_[at_c<0>(qi::_val) = orange_t::Varchar, at_c<1>(qi::_val) = qi::_1] >
                 ')') |
                kw(+"DATE")[at_c<0>(qi::_val) = orange_t::Date] |
                kw(+"FLOAT")[at_c<0>(qi::_val) = orange_t::Numeric,
                             at_c<1>(qi::_val) = 40 * 18 + 2] |
                kw(+"NUMERIC")[at_c<0>(qi::_val) = orange_t::Numeric] >> '(' >>
                    qi::int_[at_c<1>(qi::_val) = qi::_1] >> ',' >>
                    qi::int_[at_c<2>(qi::_val) = qi::_1] >> ')' |
                kw(+"NUMERIC")[at_c<0>(qi::_val) = orange_t::Numeric,
                               at_c<1>(qi::_val) = 40 * 18 + 2];

            // <value> := <int> | <string> | <float> | 'NULL'
            value = (qi::int_[at_c<0>(qi::_val) = qi::_1] >> &!qi::no_skip['.']) |
                    value_string[at_c<0>(qi::_val) = qi::_1] |
                    qi::double_[at_c<0>(qi::_val) = construct<numeric_t>(qi::_1)] | kw(+"NULL");

            // <values> := <value> (',' <value>)*
            values %= value % ',';

            // <values_list> := '(' <values> ')' (',' '(' <values> ')')*
            values_list %= ('(' > values > ')') % ',';

            // <expr> := <value> | <col>
            expression %= value | col;

            // <where> := <where_op> | <where_null>
            where %= where_op | where_null | where_like;
            // <where_op> := <col> <op> <expr>
            where_op = col[at_c<0>(qi::_val) = qi::_1] >> operator_[at_c<1>(qi::_val) = qi::_1] >
                       expression[at_c<2>(qi::_val) = qi::_1];
            // <where_null> := <col> 'IS' 'NOT'? 'NULL'
            where_null %= col >> (kw(+"IS") > qi::matches[kw(+"NOT")] > kw(+"NULL"));
            // <where_like> := <col> 'LIKE' [pattern]
            where_like %= col >> kw(+"LIKE") >> value_string;

            // <where_clause> := <where> ('AND' <where>)*
            where_list %= where % ',';

            // <set> := [col_name] '=' <value>
            set %= identifier > '=' > value;

            // <set_clause> := <set> (',' <set>)*
            set_list %= set % ',';

            // <field> := field_def | field_primary_key | field_foreign_key
            field %= definition_field | primary_key_field | foreign_key_field;
            // <field_def> := [col_name] <type> ('NOT' 'NULL')? ('DEFAULT' <value>)?
            definition_field %= identifier > type > qi::matches[kw(+"NOT") > kw(+"NULL")] >
                                -(kw(+"DEFAULT") > value);
            // <field_primary_key> := 'PRIMARY' 'KEY' '(' <column_list> ')'
            primary_key_field %= kw(+"PRIMARY") > kw(+"KEY") > '(' > columns > ')';
            // <field_foreign_key> := 'FOREIGN' 'KEY' '(' [col_name] ')' 'REFERENCES' [tb_name]
            //                        '(' [col_name] ')'
            foreign_key_field %= kw(+"PRIMARY") > kw(+"KEY") > '(' > identifier > ')' >
                                 kw(+"REFERENCES") > identifier > '(' > identifier > ')';
            // <fields> := <field> (',' <field>)*
            fields %= field % ',';

#ifndef NDEBUG
            BOOST_SPIRIT_DEBUG_NODES((program)(stmt));
            BOOST_SPIRIT_DEBUG_NODES((sys)(show_db));
            BOOST_SPIRIT_DEBUG_NODES((db)(show_tb)(create_db)(drop_db)(use_db));
            BOOST_SPIRIT_DEBUG_NODES((tb)(create_tb)(drop_tb)(desc_tb)(insert_into_tb)(
                delete_from_tb)(update_tb)(select_tb));
            BOOST_SPIRIT_DEBUG_NODES((idx)(create_idx)(drop_idx)(alter_add_idx)(alter_drop_idx));
            BOOST_SPIRIT_DEBUG_NODES((alter)(add_field)(drop_col)(change_col)(rename_tb)(
                add_primary_key)(add_constraint_primary_key)(drop_primary_key)(
                add_constraint_foreign_key)(drop_foreign_key));
            BOOST_SPIRIT_DEBUG_NODES((kw)(identifier)(value_string));
            BOOST_SPIRIT_DEBUG_NODES((col)(columns)(tables)(selectors));
            BOOST_SPIRIT_DEBUG_NODES((operator_)(type)(value)(values)(values_list)(expression));
            BOOST_SPIRIT_DEBUG_NODES((where)(where_op)(where_null)(where_list));
            BOOST_SPIRIT_DEBUG_NODES((set)(set_list));
            BOOST_SPIRIT_DEBUG_NODES(
                (field)(definition_field)(primary_key_field)(foreign_key_field)(fields));
#endif
        }
    };

    sql_ast sql_parser::parse(const std::string& sql) {
        std::setlocale(LC_ALL, "zh_CN.UTF8");

        using iterator = std::string::const_iterator;
        iterator iter = sql.begin(), end = sql.end();
        sql_grammer<iterator> parser;
        sql_ast ast;

        try {
            qi::phrase_parse(iter, end, parser, encoding::space, ast);
        }
        catch (const qi::expectation_failure<iterator>& e) {
            iterator begin = sql.begin();
            throw parse_error("parse error", std::distance(begin, e.first),
                              std::distance(begin, e.last), e.what_.tag);
        }
        return ast;
    }

#ifdef BOOST_SPIRIT_DEBUG
    // 这些是输出语法树用的
    std::ostream& operator<<(std::ostream& os, const show_tb_stmt& stmt) {
        return os << "<show_tb_stmt>";
    }
    std::ostream& operator<<(std::ostream& os, const show_db_stmt& stmt) {
        return os << "<show_db_stmt>";
    }
    std::ostream& operator<<(std::ostream& os, const data_value_kind& kind) {
        switch (kind) {
            case data_value_kind::Int:
                os << "INT";
                break;
                //                case data_value_kind::VarChar: os << "VARCHAR"; break;
                //                case data_value_kind::Date: os << "DATE"; break;
            case data_value_kind::Float: os << "FLOAT"; break;
        }
        return os;
    }
#endif  // BOOST_SPIRIT_DEBUG

}  // namespace Orange::parser
