#pragma once

#include <boost/optional.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <string>
#include <variant>
#include <vector>

namespace Orange {
    namespace parser {
        /** column */
        struct column {
            boost::optional<std::string> table_name;
            std::string col_name;
        };

        using column_list = std::vector<std::string>;  // 注意里面不是column

        /** selector: empty时代表 '*' */
        using selector = std::vector<column>;  // 注意里面是column

        using table_list = std::vector<std::string>;

        /** op: '=', '<>', '<=', '>=', '<', '>' */
        enum class op { Eq, Neq, Le, Ge, Ls, Gt };

        /** type */
        enum class DataTypeKind { Int, VarChar, Date, Float };
        struct data_type {
            DataTypeKind type;
            int value;
        };

        /** value */
        using data_value = boost::variant<int, std::string>; // 暂时决定当variant为empty时代表null(出问题再说
        using data_value_list = std::vector<data_value>;
        using data_value_lists = std::vector<data_value_list>;

        /** expr */
        using expr = boost::variant<data_value, column>;

        /** field */
        enum class FieldKind { Def, PrimaryKey, ForeignKey };

        struct field_def {
            std::string col_name;
            data_type type;
            bool is_null;
            boost::optional<data_value> default_value;
        };

        struct field_primary_key {
            column_list col_list;
        };

        struct field_foreign_key {
            std::string col;
            std::string ref_table_name;
            std::string ref_col_name;
        };

        using single_field = boost::variant<field_def, field_primary_key, field_foreign_key>;
        using field_list = std::vector<single_field>;

        /** where clause */
        struct single_where_op {
            std::string col_name;
            op operator_;
            expr expression;
        };

        struct single_where_null {
            std::string col_name;
            bool is_null;
        };

        using single_where = boost::variant<single_where_op, single_where_null>;

        using where_clause = std::vector<single_where>;

        /** set clause */
        struct single_set {
            std::string col_name;
            data_value val;
        };

        using set_clause = std::vector<single_set>;

        /** sql statement */
        enum class StmtKind { Sys, Db, Tb, Idx, Alter };

        /** system statement */
        enum class SysStmtKind { ShowDb };

        struct show_db_stmt {};

        using sys_stmt = boost::variant<show_db_stmt>;

        /** database statement */
        enum class DbStmtKind { Show, Create, Drop, Use };

        struct show_tb_stmt {};

        struct create_db_stmt {
            std::string name;
        };

        struct drop_db_stmt {
            std::string name;
        };

        struct use_db_stmt {
            std::string name;
        };

        using db_stmt = boost::variant<show_tb_stmt, create_db_stmt, drop_db_stmt, use_db_stmt>;

        /** table statement */
        enum class TbStmtKind {
            Create,
            Drop,
            Desc,
            InsertInto,
            DeleteFrom,
            Update,
            Select
        };

        struct create_tb_stmt {
            std::string name;
            field_list fields;
        };

        struct drop_tb_stmt {
            std::string name;
        };

        struct desc_tb_stmt {
            std::string name;
        };

        struct insert_into_tb_stmt {
            std::string name;
            data_value_lists values;
        };

        struct delete_from_tb_stmt {
            std::string name;
            where_clause where_clause;
        };

        struct update_tb_stmt {
            std::string name;
            set_clause set;
            where_clause where;
        };

        struct select_tb_stmt {
            selector select;
            table_list tables;
            where_clause where;
        };

        using tb_stmt = boost::variant<create_tb_stmt, drop_tb_stmt, desc_tb_stmt, insert_into_tb_stmt, delete_from_tb_stmt, update_tb_stmt, select_tb_stmt>;

        /** index statement */
        enum class IdxStmtKind { None = -1, Create, Drop, AlterAdd, AlterDrop };

        struct create_idx_stmt {
            std::string idx_name;
            std::string tb_name;
            column_list col_list;
        };

        struct drop_idx_stmt {
            std::string name;
        };

        struct alter_add_idx_stmt {
            std::string tb_name;
            std::string idx_name;
            column_list col_list;
        };

        struct alter_drop_idx_stmt {
            std::string tb_name;
            std::string idx_name;
        };

        using idx_stmt = boost::variant<create_idx_stmt, drop_idx_stmt, alter_add_idx_stmt,
                                             alter_drop_idx_stmt>;

        /** alter statement */
        enum class AlterStmtKind {
            AddField,
            DropCol,
            ChangeCol,
            RenameTb,
            AddPrimaryKey,
            AddConstraintPrimaryKey,
            DropPrimaryKey,
            AddConstraintForeignKey,
            DropForeignKey
        };

        struct add_field_stmt {
            std::string table_name;
            single_field new_field;
        };

        struct drop_col_stmt {
            std::string table_name;
            std::string col_name;
        };

        struct change_col_stmt {
            std::string table_name;
            std::string col_name;
            single_field new_field;
        };

        struct rename_tb_stmt {
            std::string table_name;
            std::string new_tb_name;
        };

        struct add_primary_key_stmt {
            std::string table_name;
            column_list col_list;
        };

        struct add_constraint_primary_key_stmt {
            std::string table_name;
            std::string pk_name;
            column_list col_list;
        };

        struct drop_primary_key_stmt {
            std::string table_name;
            boost::optional<std::string> pk_name;
        };

        struct add_constraint_foreign_key_stmt {
            std::string table_name;
            std::string fk_name;
            column_list col_list;
            std::string ref_tb_name;
            column_list ref_col_list;
        };

        struct drop_foreign_key_stmt {
            std::string table_name;
            std::string fk_name;
        };

        using alter_stmt = boost::variant<add_field_stmt, drop_col_stmt, change_col_stmt,
                                          rename_tb_stmt, add_primary_key_stmt,
                                          add_constraint_primary_key_stmt, drop_primary_key_stmt,
                                          add_constraint_foreign_key_stmt, drop_foreign_key_stmt>;

        /** sql statmemet */
        using sql_stmt = boost::variant<sys_stmt, db_stmt, tb_stmt, idx_stmt, alter_stmt>;

        using sql_stmt_list = std::vector<sql_stmt>;

        /** ast */
        struct sql_ast {
            sql_stmt_list stmt;
        };
    }  // namespace parser
}  // namespace Orange
