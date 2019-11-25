#pragma once

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Orange {

    /** column */
    struct column {
        std::optional<std::string> table_name;
        std::string col_name;
    };

    /** selector: 定义empty时代表 '*' */
    using selector = std::vector<column>;

    using column_list = std::vector<std::string>;  // list不能带table_name

    using table_list = std::vector<std::string>;

    /** op: '=', '<>', '<=', '>=', '<', '>' */
    enum class op { None = -1, Eq, Neq, Le, Ge, Ls, Gt };

    /** type */
    struct data_type {
        enum { Int, VarChar, Date, Float };
        int value;
    };

    /** value */
    using value = std::variant<int, std::string, float>;

    using value_list = std::vector<value>;

    using value_lists = std::vector<value_list>;

    /** expr */
    using expr = std::variant<value, column>;


    /** field */
    enum class FieldKind { None = -1, Def, PrimaryKey, ForeignKey };
    struct field {
        FieldKind kind;
        field() : kind(FieldKind::None) {}
        field(FieldKind kind) : kind(kind) {}
    };

    struct field_def : field {
        std::string col_name;
        data_type type;
        bool is_null;
        std::optional<value> default_value;

        field_def() : field(FieldKind::Def), col_name(), type(), is_null(), default_value() {}
    };

    struct field_primary_key : field {
        column_list col_list;
        field_primary_key() : field(FieldKind::PrimaryKey), col_list() {}
    };

    struct field_foreign_key : field {
        std::string col;
        std::string ref_table_name;
        std::string ref_col_name;
        field_foreign_key() :
            field(FieldKind::ForeignKey), col(), ref_table_name(), ref_col_name() {}
    };

    using field_list = std::vector<field>;

    /** where clause */
    struct where_clause {
        std::string col_name;
        std::variant<std::pair<op, expr>, bool> cond;
    };

    using where_clause_list = std::vector<where_clause>;

    /** set clause */
    struct set_clause {
        std::string col_name;
    };

    /** sql statement */
    enum class StmtKind { None = -1, Sys, Db, Tb, Idx, Alter };
    struct sql_stmt {
        StmtKind stmt_kind;
        sql_stmt() : stmt_kind(StmtKind::None) {}
        sql_stmt(StmtKind kind) : stmt_kind(kind) {}
    };

    using sql_stmt_list = std::vector<sql_stmt>;

    /** system statement */
    enum class SysStmtKind { None = -1, ShowDb };
    struct sys_stmt : sql_stmt {
        SysStmtKind sys_stmt_kind;
        sys_stmt() : sql_stmt(StmtKind::Sys), sys_stmt_kind(SysStmtKind::None) {}
        sys_stmt(SysStmtKind kind) : sql_stmt(StmtKind::Sys), sys_stmt_kind(kind) {}
    };

    struct show_db_stmt : sys_stmt {
        show_db_stmt() : sys_stmt(SysStmtKind::ShowDb) {}
    };

    /** database statement */
    enum class DbStmtKind { None = -1, Show, Create, Drop, Use };
    struct db_stmt : sql_stmt {
        DbStmtKind db_stmt_kind;
        db_stmt() : sql_stmt(StmtKind::Db), db_stmt_kind(DbStmtKind::None) {}
        db_stmt(DbStmtKind kind) : sql_stmt(StmtKind::Db), db_stmt_kind(kind) {}
    };

    struct show_tb_stmt : db_stmt {
        show_tb_stmt() : db_stmt(DbStmtKind::Show) {}
    };

    struct create_db_stmt : db_stmt {
        std::string name;
        create_db_stmt() : db_stmt(DbStmtKind::Create), name() {}
        create_db_stmt(const std::string& name) : db_stmt(DbStmtKind::Create), name(name) {}
    };

    struct drop_db_stmt : db_stmt {
        std::string name;
        drop_db_stmt() : db_stmt(DbStmtKind::Drop), name() {}
        drop_db_stmt(const std::string& name) : db_stmt(DbStmtKind::Drop), name(name) {}
    };

    struct use_db_stmt : db_stmt {
        std::string name;
        use_db_stmt() : db_stmt(DbStmtKind::Use), name() {}
        use_db_stmt(const std::string& name) : db_stmt(DbStmtKind::Use), name(name) {}
    };

    /** table statement */
    enum class TbStmtKind { None = -1, Create, Drop, Desc, InsertInto, DeleteFrom, Update, Select };
    struct tb_stmt : sql_stmt {
        TbStmtKind tb_stmt_kind;
        tb_stmt() : sql_stmt(StmtKind::Tb), tb_stmt_kind(TbStmtKind::None) {}
        tb_stmt(TbStmtKind kind) : sql_stmt(StmtKind::Tb), tb_stmt_kind(kind) {}
    };

    struct create_tb_stmt : tb_stmt {
        std::string name;
        field_list fields;
        create_tb_stmt() : tb_stmt(TbStmtKind::Create), name(), fields() {}
        create_tb_stmt(const std::string& name, const field_list& fields) :
            tb_stmt(TbStmtKind::Create), name(name), fields(fields) {}
    };

    struct drop_tb_stmt : tb_stmt {
        std::string name;
        drop_tb_stmt() : tb_stmt(TbStmtKind::Drop), name() {}
    };

    struct desc_tb_stmt : tb_stmt {
        std::string name;
        desc_tb_stmt() : tb_stmt(TbStmtKind::Desc), name() {}
    };

    struct insert_into_tb_stmt : tb_stmt {
        std::string name;
        value_list values;
        insert_into_tb_stmt() : tb_stmt(TbStmtKind::InsertInto), name(), values() {}
    };

    struct delete_from_tb_stmt : tb_stmt {
        std::string name;
        where_clause_list where_clause;
        delete_from_tb_stmt() : tb_stmt(TbStmtKind::DeleteFrom), name(), where_clause() {}
    };

    struct update_tb_stmt : tb_stmt {
        std::string name;
        set_clause set;
        where_clause where;
    };

    struct select_tb_stmt : tb_stmt {
        selector sel;
        table_list tables;
        where_clause where;
    };

    /** index statement */
    enum class IdxStmtKind { None = -1, Create, Drop, AlterAdd, AlterDrop };
    struct idx_stmt : sql_stmt {
        IdxStmtKind idx_stmt_kind;
        idx_stmt() : sql_stmt(StmtKind::Idx) {}
        idx_stmt(IdxStmtKind kind) : sql_stmt(StmtKind::Idx), idx_stmt_kind(kind) {}
    };

    struct create_idx_stmt : idx_stmt {
        std::string idx_name;
        std::string tb_name;
        column_list col_list;
        create_idx_stmt() : idx_stmt(IdxStmtKind::Create), idx_name(), tb_name(), col_list() {}
    };

    struct drop_idx_stmt : idx_stmt {
        std::string name;
        drop_idx_stmt() : idx_stmt(IdxStmtKind::Drop), name() {}
    };

    struct alter_add_idx_stmt : idx_stmt {
        std::string tb_name;
        std::string idx_name;
        column_list col_list;
        alter_add_idx_stmt() : idx_stmt(IdxStmtKind::AlterAdd), tb_name(), idx_name(), col_list() {}
    };

    struct alter_drop_idx_stmt : idx_stmt {
        std::string tb_name;
        std::string idx_name;
        alter_drop_idx_stmt() : idx_stmt(IdxStmtKind::AlterDrop), tb_name(), idx_name() {}
    };

    /** alter statement */
    enum class AlterStmtKind {
        None = -1,
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
    struct alter_stmt : sql_stmt {
        std::string tb_name;
        AlterStmtKind alter_stmt_kind;
        alter_stmt() : sql_stmt(StmtKind::Alter), tb_name(), alter_stmt_kind(AlterStmtKind::None) {}
        alter_stmt(AlterStmtKind kind) :
            sql_stmt(StmtKind::Alter), tb_name(), alter_stmt_kind(kind) {}
    };

    struct add_field_stmt : alter_stmt {
        field f;
        add_field_stmt() : alter_stmt(AlterStmtKind::AddField) {}
    };

    struct drop_col_stmt : alter_stmt {
        std::string col_name;
        drop_col_stmt() : alter_stmt(AlterStmtKind::DropCol) {}
    };

    struct change_col_stmt : alter_stmt {
        std::string col_name;
        field f;
        change_col_stmt() : alter_stmt(AlterStmtKind::ChangeCol) {}
    };

    struct rename_tb_stmt : alter_stmt {
        std::string new_tb_name;
        rename_tb_stmt() : alter_stmt(AlterStmtKind::RenameTb) {}
    };

    struct add_primary_key_stmt : alter_stmt {
        column_list col_list;
        add_primary_key_stmt() : alter_stmt(AlterStmtKind::AddPrimaryKey) {}
    };

    struct add_constraint_primary_key_stmt : alter_stmt {
        std::string pk_name;
        column_list col_list;
        add_constraint_primary_key_stmt() : alter_stmt(AlterStmtKind::AddConstraintPrimaryKey) {}
    };

    struct drop_primary_key_stmt : alter_stmt {
        std::optional<std::string> pk_name;
        drop_primary_key_stmt() : alter_stmt(AlterStmtKind::DropPrimaryKey) {}
    };

    struct add_constraint_foreign_key_stmt : alter_stmt {
        std::string fk_name;
        column_list col_list;
        std::string ref_tb_name;
        std::string ref_col_list;
        add_constraint_foreign_key_stmt() : alter_stmt(AlterStmtKind::AddConstraintForeignKey) {}
    };

    struct drop_foreign_key_stmt : alter_stmt {
        std::string fk_name;
        drop_foreign_key_stmt() : alter_stmt(AlterStmtKind::DropForeignKey) {}
    };


    /** sql */
    struct SQL {
        sql_stmt_list stmt;
    };
}  // namespace Orange
