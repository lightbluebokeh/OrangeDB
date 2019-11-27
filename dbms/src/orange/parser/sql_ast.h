#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4828)
#endif

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/variant.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <vector>

namespace Orange {
    namespace parser {
        // 这些都是ast结点
        // 使用boost::optional和boost::variant, 因为它们不能转成std的

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
            DataTypeKind kind;
            int value;
        };

        /** value */
        struct data_value {
            boost::variant<int, std::string> value;

            bool is_int() const { return value.which() == 0; }
            bool is_string() const { return value.which() == 1; }
            bool is_null() const { return value.empty(); }

            int& to_int() { return boost::get<int>(value); }
            int to_int() const { return boost::get<int>(value); }
            std::string& to_string() { return boost::get<std::string>(value); }
            const std::string& to_string() const { return boost::get<std::string>(value); }

            // 一个奇怪的编译错误
            operator boost::variant<int, std::string>() const { return value; }
        };
        using data_value_list = std::vector<data_value>;
        using data_value_lists = std::vector<data_value_list>;

        /** expr */
        struct expr {
            boost::variant<data_value, column> expression;

            bool is_value() const { return expression.which() == 0; }
            bool is_column() const { return expression.which() == 1; }

            data_value& value() { return boost::get<data_value>(expression); }
            column& col() { return boost::get<column>(expression); }
        };

        /** field */
        enum class FieldKind { Def, PrimaryKey, ForeignKey };

        struct field_def {
            std::string col_name;
            data_type type;
            bool is_not_null;
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

        struct single_field {
            boost::variant<field_def, field_primary_key, field_foreign_key> field;

            FieldKind kind() const { return (FieldKind)field.which(); }

            field_def& def() { return boost::get<field_def>(field); }
            field_primary_key& primary_key() { return boost::get<field_primary_key>(field); }
            field_foreign_key& foreign_key() { return boost::get<field_foreign_key>(field); }
        };
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

        struct single_where {
            boost::variant<single_where_op, single_where_null> where;

            bool is_op() const { return where.which() == 0; }
            bool is_is_null() const { return where.which() == 1; }

            single_where_op& op() { return boost::get<single_where_op>(where); }
            single_where_null& null() { return boost::get<single_where_null>(where); }
        };

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

        struct sys_stmt {
            boost::variant<show_db_stmt> stmt;

            SysStmtKind kind() const { return (SysStmtKind)stmt.which(); }

            show_db_stmt& show_db() { return boost::get<show_db_stmt>(stmt); }
        };

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

        struct db_stmt {
            boost::variant<show_tb_stmt, create_db_stmt, drop_db_stmt, use_db_stmt> stmt;

            DbStmtKind kind() const { return (DbStmtKind)stmt.which(); }

            show_tb_stmt& show() { return boost::get<show_tb_stmt>(stmt); }
            create_db_stmt& create() { return boost::get<create_db_stmt>(stmt); }
            drop_db_stmt& drop() { return boost::get<drop_db_stmt>(stmt); }
            use_db_stmt& use() { return boost::get<use_db_stmt>(stmt); }
        };

        /** table statement */
        enum class TbStmtKind { Create, Drop, Desc, InsertInto, DeleteFrom, Update, Select };

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

        struct tb_stmt {
            boost::variant<create_tb_stmt, drop_tb_stmt, desc_tb_stmt, insert_into_tb_stmt,
                           delete_from_tb_stmt, update_tb_stmt, select_tb_stmt>
                stmt;

            TbStmtKind kind() const { return (TbStmtKind)stmt.which(); }

            create_tb_stmt& create() { return boost::get<create_tb_stmt>(stmt); }
            drop_tb_stmt& drop() { return boost::get<drop_tb_stmt>(stmt); }
            desc_tb_stmt& desc() { return boost::get<desc_tb_stmt>(stmt); }
            insert_into_tb_stmt& insert_into() { return boost::get<insert_into_tb_stmt>(stmt); }
            delete_from_tb_stmt& delete_from() { return boost::get<delete_from_tb_stmt>(stmt); }
            update_tb_stmt& update() { return boost::get<update_tb_stmt>(stmt); }
            select_tb_stmt& select() { return boost::get<select_tb_stmt>(stmt); }
        };

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

        struct idx_stmt {
            boost::variant<create_idx_stmt, drop_idx_stmt, alter_add_idx_stmt, alter_drop_idx_stmt>
                stmt;

            IdxStmtKind kind() const { return (IdxStmtKind)stmt.which(); }
            create_idx_stmt& create() { return boost::get<create_idx_stmt>(stmt); }
            drop_idx_stmt& drop() { return boost::get<drop_idx_stmt>(stmt); }
            alter_add_idx_stmt& alter_add() { return boost::get<alter_add_idx_stmt>(stmt); }
            alter_drop_idx_stmt& alter_drop() { return boost::get<alter_drop_idx_stmt>(stmt); }
        };

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

        struct alter_stmt {
            boost::variant<add_field_stmt, drop_col_stmt, change_col_stmt, rename_tb_stmt,
                           add_primary_key_stmt, add_constraint_primary_key_stmt,
                           drop_primary_key_stmt, add_constraint_foreign_key_stmt,
                           drop_foreign_key_stmt>
                stmt;

            AlterStmtKind kind() const { return (AlterStmtKind)stmt.which(); }

            add_field_stmt& add_field() { return boost::get<add_field_stmt>(stmt); }
            drop_col_stmt& drop_col() { return boost::get<drop_col_stmt>(stmt); }
            change_col_stmt& change_col() { return boost::get<change_col_stmt>(stmt); }
            rename_tb_stmt& rename_tb() { return boost::get<rename_tb_stmt>(stmt); }
            add_primary_key_stmt& add_primary_key() {
                return boost::get<add_primary_key_stmt>(stmt);
            }
            add_constraint_primary_key_stmt& add_constraint_primary_key() {
                return boost::get<add_constraint_primary_key_stmt>(stmt);
            }
            drop_primary_key_stmt& drop_primary_key() {
                return boost::get<drop_primary_key_stmt>(stmt);
            }
            add_constraint_foreign_key_stmt& add_constraint_foreign_key() {
                return boost::get<add_constraint_foreign_key_stmt>(stmt);
            }
            drop_foreign_key_stmt& drop_foreign_key() {
                return boost::get<drop_foreign_key_stmt>(stmt);
            }
        };

        /** sql statement */
        struct sql_stmt {
            boost::variant<sys_stmt, db_stmt, tb_stmt, idx_stmt, alter_stmt> stmt;

            // 包装的语句类型
            StmtKind kind() const { return (StmtKind)stmt.which(); }

            // 转换成特定的语句，类型不对应时get会抛异常
            sys_stmt& sys() { return boost::get<sys_stmt>(stmt); }
            db_stmt& db() { return boost::get<db_stmt>(stmt); }
            tb_stmt& tb() { return boost::get<tb_stmt>(stmt); }
            idx_stmt& idx() { return boost::get<idx_stmt>(stmt); }
            alter_stmt& alter() { return boost::get<alter_stmt>(stmt); }
        };

        using sql_stmt_list = std::vector<sql_stmt>;

        /** ast */
        struct sql_ast {
            // 语句列表
            sql_stmt_list stmt_list;
        };
    }  // namespace parser
}  // namespace Orange
