#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4828)
#endif

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "defs.h"

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
        enum class op { Eq, Neq, Le, Ge, Lt, Gt };

        /** type */
        // enum class DataTypeKind { Int, VarChar, Date, Float };
        struct data_type {
            orange_t kind;
            boost::variant<boost::blank, int_t> value;

            bool has_value() const { return value.which() != 0; }

            int_t& int_value() { return boost::get<int_t>(value); }
            int_t int_value() const { return boost::get<int_t>(value); }
            int_t int_value_or(int_t x) const { return has_value() ? int_value() : x; }
        };

        inline std::ostream& operator << (ostream& os, const data_type& type) {
            os << type.kind << ' ' << type.int_value_or(-1);    // cannot be negative
            return os;
        }
        inline std::istream& operator >> (istream& is, data_type& type) {
            is >> type.kind;
            int value;
            is >> value;
            if (value == -1) type.value = boost::blank();
            else type.value = value;
            return is;
        }

        /** value */
        enum class data_value_kind { Null, Int, String, Float };
        struct data_value {
            using value_type = boost::variant<boost::blank, int_t, std::string, numeric_t>;
            value_type value;

            bool is_null() const { return value.which() == 0; }
            bool is_int() const { return value.which() == 1; }
            bool is_string() const { return value.which() == 2; }
            bool is_float() const { return value.which() == 3; }
            data_value_kind kind() const { return (data_value_kind)value.which(); }

            int_t& to_int() { return boost::get<int_t>(value); }
            int_t to_int() const { return boost::get<int_t>(value); }
            std::string& to_string() { return boost::get<std::string>(value); }
            const std::string& to_string() const { return boost::get<std::string>(value); }
            numeric_t& to_float() { return boost::get<numeric_t>(value); }
            numeric_t to_float() const { return boost::get<numeric_t>(value); }

            static data_value from_null() { return data_value{}; }
            static data_value from_int(int_t x) { return data_value{x}; }
            static data_value from_float(numeric_t f) { return data_value{f}; }
            static data_value from_string(std::string s) { return data_value{s}; }

            operator value_type() const { return value; }
        };

        inline std::istream& operator >> (std::istream& is, data_value& value) {
            int which;
            is >> which;
            switch (data_value_kind(which)) {
                case data_value_kind::Null: // do nothing
                break;
                case data_value_kind::Int: is >> value.to_int();
                break;
                case data_value_kind::String: is >> value.to_string();
                break;
                case data_value_kind::Float: is >> value.to_float();
                break;
            }
            return is;
        }
        inline std::ostream& operator << (std::ostream& os, const data_value& value) {
            os << value.value.which();
            switch (data_value_kind(value.value.which())) {
                case data_value_kind::Null: // do  nothing
                break;
                case data_value_kind::Int: os << ' ' << value.to_int();
                break;
                case data_value_kind::String: os << ' ' << value.to_string();
                break;
                case data_value_kind::Float: os << ' ' << value.to_float();
                break;
            }
            return os;
        }

        using data_value_list = std::vector<data_value>;
        using data_value_lists = std::vector<data_value_list>;  // 这个东西多半是假的

        /** expr */
        struct expr {
            boost::variant<data_value, column> expression;

            bool is_value() const { return expression.which() == 0; }
            bool is_column() const { return expression.which() == 1; }

            const data_value& value() const { return boost::get<data_value>(expression); }
            data_value& value() { return boost::get<data_value>(expression); }
            const column& col() const { return boost::get<column>(expression); }
            column& col() { return boost::get<column>(expression); }
        };

        /** field */
        enum class FieldKind { Def, PrimaryKey, ForeignKey };

        struct field_def {
            std::string col_name;
            data_type type;
            bool is_not_null;
            // bool is_primary_key;  // 有需求再加
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

            const field_def& def() const { return boost::get<field_def>(field); }
            field_def& def() { return boost::get<field_def>(field); }
            const field_primary_key& primary_key() const {
                return boost::get<field_primary_key>(field);
            }
            field_primary_key& primary_key() { return boost::get<field_primary_key>(field); }
            const field_foreign_key& foreign_key() const {
                return boost::get<field_foreign_key>(field);
            }
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
            bool is_not_null;
        };

        struct single_where {
            boost::variant<single_where_op, single_where_null> where;

            bool is_op() const { return where.which() == 0; }
            bool is_null_check() const { return where.which() == 1; }

            const single_where_op& op() const { return boost::get<single_where_op>(where); }
            single_where_op& op() { return boost::get<single_where_op>(where); }
            const single_where_null& null_check() const {
                return boost::get<single_where_null>(where);
            }
            single_where_null& null_check() { return boost::get<single_where_null>(where); }
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

            const show_db_stmt& show_db() const { return boost::get<show_db_stmt>(stmt); }
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

            const show_tb_stmt& show() const { return boost::get<show_tb_stmt>(stmt); }
            show_tb_stmt& show() { return boost::get<show_tb_stmt>(stmt); }
            const create_db_stmt& create() const { return boost::get<create_db_stmt>(stmt); }
            create_db_stmt& create() { return boost::get<create_db_stmt>(stmt); }
            const drop_db_stmt& drop() const { return boost::get<drop_db_stmt>(stmt); }
            drop_db_stmt& drop() { return boost::get<drop_db_stmt>(stmt); }
            const use_db_stmt& use() const { return boost::get<use_db_stmt>(stmt); }
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
            boost::optional<column_list> columns;
            data_value_lists value_lists;
        };

        struct delete_from_tb_stmt {
            std::string name;
            where_clause where;
        };

        struct update_tb_stmt {
            std::string name;
            set_clause set;
            where_clause where;
        };

        struct select_tb_stmt {
            selector select;
            table_list tables;
            boost::optional<where_clause> where;
        };

        struct tb_stmt {
            boost::variant<create_tb_stmt, drop_tb_stmt, desc_tb_stmt, insert_into_tb_stmt,
                           delete_from_tb_stmt, update_tb_stmt, select_tb_stmt>
                stmt;

            TbStmtKind kind() const { return (TbStmtKind)stmt.which(); }

            const create_tb_stmt& create() const { return boost::get<create_tb_stmt>(stmt); }
            create_tb_stmt& create() { return boost::get<create_tb_stmt>(stmt); }
            const drop_tb_stmt& drop() const { return boost::get<drop_tb_stmt>(stmt); }
            drop_tb_stmt& drop() { return boost::get<drop_tb_stmt>(stmt); }
            const desc_tb_stmt& desc() const { return boost::get<desc_tb_stmt>(stmt); }
            desc_tb_stmt& desc() { return boost::get<desc_tb_stmt>(stmt); }
            const insert_into_tb_stmt& insert_into() const {
                return boost::get<insert_into_tb_stmt>(stmt);
            }
            insert_into_tb_stmt& insert_into() { return boost::get<insert_into_tb_stmt>(stmt); }
            const delete_from_tb_stmt& delete_from() const {
                return boost::get<delete_from_tb_stmt>(stmt);
            }
            delete_from_tb_stmt& delete_from() { return boost::get<delete_from_tb_stmt>(stmt); }
            const update_tb_stmt& update() const { return boost::get<update_tb_stmt>(stmt); }
            update_tb_stmt& update() { return boost::get<update_tb_stmt>(stmt); }
            const select_tb_stmt& select() const { return boost::get<select_tb_stmt>(stmt); }
            select_tb_stmt& select() { return boost::get<select_tb_stmt>(stmt); }
        };

        /** index statement */
        enum class IdxStmtKind { Create, Drop, AlterAdd, AlterDrop };

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

            const create_idx_stmt& create() const { return boost::get<create_idx_stmt>(stmt); }
            create_idx_stmt& create() { return boost::get<create_idx_stmt>(stmt); }
            const drop_idx_stmt& drop() const { return boost::get<drop_idx_stmt>(stmt); }
            drop_idx_stmt& drop() { return boost::get<drop_idx_stmt>(stmt); }
            const alter_add_idx_stmt& alter_add() const {
                return boost::get<alter_add_idx_stmt>(stmt);
            }
            alter_add_idx_stmt& alter_add() { return boost::get<alter_add_idx_stmt>(stmt); }
            const alter_drop_idx_stmt& alter_drop() const {
                return boost::get<alter_drop_idx_stmt>(stmt);
            }
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

            const add_field_stmt& add_field() const { return boost::get<add_field_stmt>(stmt); }
            add_field_stmt& add_field() { return boost::get<add_field_stmt>(stmt); }
            const drop_col_stmt& drop_col() const { return boost::get<drop_col_stmt>(stmt); }
            drop_col_stmt& drop_col() { return boost::get<drop_col_stmt>(stmt); }
            const change_col_stmt& change_col() const { return boost::get<change_col_stmt>(stmt); }
            change_col_stmt& change_col() { return boost::get<change_col_stmt>(stmt); }
            const rename_tb_stmt& rename_tb() const { return boost::get<rename_tb_stmt>(stmt); }
            rename_tb_stmt& rename_tb() { return boost::get<rename_tb_stmt>(stmt); }
            const add_primary_key_stmt& add_primary_key() const {
                return boost::get<add_primary_key_stmt>(stmt);
            }
            add_primary_key_stmt& add_primary_key() {
                return boost::get<add_primary_key_stmt>(stmt);
            }
            const add_constraint_primary_key_stmt& add_constraint_primary_key() const {
                return boost::get<add_constraint_primary_key_stmt>(stmt);
            }
            add_constraint_primary_key_stmt& add_constraint_primary_key() {
                return boost::get<add_constraint_primary_key_stmt>(stmt);
            }
            const drop_primary_key_stmt& drop_primary_key() const {
                return boost::get<drop_primary_key_stmt>(stmt);
            }
            drop_primary_key_stmt& drop_primary_key() {
                return boost::get<drop_primary_key_stmt>(stmt);
            }
            const add_constraint_foreign_key_stmt& add_constraint_foreign_key() const {
                return boost::get<add_constraint_foreign_key_stmt>(stmt);
            }
            add_constraint_foreign_key_stmt& add_constraint_foreign_key() {
                return boost::get<add_constraint_foreign_key_stmt>(stmt);
            }
            const drop_foreign_key_stmt& drop_foreign_key() const {
                return boost::get<drop_foreign_key_stmt>(stmt);
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

            // 转换成特定的语句，类型不对应时会抛异常
            const sys_stmt& sys() const { return boost::get<sys_stmt>(stmt); }
            sys_stmt& sys() { return boost::get<sys_stmt>(stmt); }
            const db_stmt& db() const { return boost::get<db_stmt>(stmt); }
            db_stmt& db() { return boost::get<db_stmt>(stmt); }
            const tb_stmt& tb() const { return boost::get<tb_stmt>(stmt); }
            tb_stmt& tb() { return boost::get<tb_stmt>(stmt); }
            const idx_stmt& idx() const { return boost::get<idx_stmt>(stmt); }
            idx_stmt& idx() { return boost::get<idx_stmt>(stmt); }
            const alter_stmt& alter() const { return boost::get<alter_stmt>(stmt); }
            alter_stmt& alter() { return boost::get<alter_stmt>(stmt); }
        };

        using sql_stmt_list = std::vector<sql_stmt>;

        /** ast */
        struct sql_ast {
            // 语句列表
            sql_stmt_list stmt_list;
        };

        inline std::ostream& operator<<(std::ostream& os, const op& oper) {
            switch (oper) {
                case op::Eq: os << "="; break;
                case op::Neq: os << "<>"; break;
                case op::Le: os << "<="; break;
                case op::Ge: os << ">="; break;
                case op::Lt: os << "<"; break;
                case op::Gt: os << ">"; break;
            }
            return os;
        }
    }  // namespace parser
}  // namespace Orange

namespace ast = Orange::parser;

namespace exception {
    // value 转为对应类型失败
    void value_convert(const String& msg) {
        throw OrangeException("convert value failed: " + msg);
    }
}

namespace Orange {
    // 保证可以转换
    inline byte_arr_t value_to_bytes(const ast::data_value& value, const ast::data_type& type) {
        using ast::data_value_kind;
        switch (value.kind) {
            case data_value_kind::Null: {
                byte_arr_t ret = {DATA_NULL};
                switch (type.kind) {
                    case orange_t::Char: ret.resize(type.int_value() + 1);
                    break;
                    case orange_t::Date: ORANGE_UNIMPL
                    case orange_t::Int: ret.resize(sizeof(int_t) + 1);
                    break;
                    case orange_t::Numeric: ret.resize(sizeof(numeric_t) + 1);
                    break;
                }
                return ret;
            } break;
            case data_value_kind::Int: 
                switch (type.kind) {
                    case orange_t::Int: return Orange::to_bytes(value.to_int());
                    case orange_t::Numeric: return Orange::to_bytes<numeric_t>(value.to_int());
                    default: ORANGE_UNREACHABLE
                }
            break;
            case data_value_kind::Float:
                if (type.kind == orange_t::Numeric) return Orange::to_bytes(value.to_float());
                ORANGE_UNREACHABLE
            break;
            case data_value_kind::String:
                switch (type.kind) {
                    case orange_t::Varchar:
                    case orange_t::Char: {
                        auto &str = value.to_string();
                        if (str.length() > type.int_value()) exception::value_convert("char limit exceeded");
                        auto ret = to_bytes(str);
                        if (type.kind == orange_t::Char) ret.resize(type.int_value() + 1);  // 对于 char 补齐
                        return ret;
                    } break;
                    case orange_t::Date: ORANGE_UNIMPL
                    default: ORANGE_UNREACHABLE
                }
            break;
        }
        ORANGE_UNREACHABLE
    }
}
