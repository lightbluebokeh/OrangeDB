#include "syntax.h"

namespace Orange {
    // 可能调试有用
    [[noreturn]] void unexpected() { throw std::runtime_error("unexpected error"); }

    using namespace parser;

    std::string type_string(const data_type& type) {
        switch (type.kind) {
            case DataTypeKind::Int: return "INT, " + std::to_string(type.value);
            case DataTypeKind::VarChar: return "VARCHAR, " + std::to_string(type.value);
            case DataTypeKind::Date: return "DATE";
            case DataTypeKind::Float: return "FLOAT";
            default: unexpected();
        }
    }
    std::string value_string(const data_value& value) {
        switch (value.kind()) {
            case DataValueKind::Int: return std::to_string(value.to_int());
            case DataValueKind::String: return "'" + value.to_string() + "'";
            case DataValueKind::Float: return std::to_string(value.to_float());
            case DataValueKind::Null: return "NULL";
            default: unexpected();
        }
    }
    std::string expression_string(const expr& e) {
        if (e.is_column()) {
            return (e.col().table_name.has_value() ? e.col().table_name.get() + "." : "") +
                   e.col().col_name;
        } else if (e.is_value()) {
            return value_string(e.value());
        } else
            unexpected();
    }

    /** 遍历语法树部分 */

    void sys(sys_stmt& stmt) {
        switch (stmt.kind()) {
            case SysStmtKind::ShowDb: std::cout << "show something?\n";
            default: unexpected();
        }
    }

    void db(db_stmt& stmt) {
        switch (stmt.kind()) {
            case DbStmtKind::Show: {
                std::cout << "nothing to show." << std::endl;
            } break;
            case DbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                std::cout << "  create database:" << std::endl;
                std::cout << "    name: " << create_stmt.name << std::endl;
            } break;
            case DbStmtKind::Drop: {
                auto& drop_stmt = stmt.drop();
                std::cout << "  drop database:" << std::endl;
                std::cout << "    name: " << drop_stmt.name << std::endl;
            } break;
            case DbStmtKind::Use: {
                auto& use_stmt = stmt.use();
                std::cout << "  use database:" << std::endl;
                std::cout << "    name: " << use_stmt.name << std::endl;
            } break;
            default: unexpected();
        }
    }

    void tb(tb_stmt& stmt) {
        switch (stmt.kind()) {
            case TbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                std::cout << "  create table:" << std::endl;
                std::cout << "    table: " << create_stmt.name << std::endl;
                std::cout << "    fields:" << std::endl;
                for (auto& field : create_stmt.fields) {
                    switch (field.kind()) {
                        case FieldKind::Def: {
                            auto& def = field.def();
                            std::cout << "      def:" << std::endl;
                            std::cout << "        col name: " << def.col_name << std::endl;
                            std::cout << "        type: " << type_string(def.type) << std::endl;
                            std::cout << "        is not null: " << def.is_not_null << std::endl;
                            std::cout << "        default value: "
                                      << (def.default_value.has_value()
                                              ? value_string(def.default_value.get())
                                              : "<none>")
                                      << std::endl;
                        } break;
                        case FieldKind::PrimaryKey: {
                            auto& primary_key = field.primary_key();
                            std::cout << "      primary key:" << std::endl;
                            std::cout << "        col list: ";
                            for (auto& col : primary_key.col_list) {
                                std::cout << col << ' ';
                            }
                            std::cout << std::endl;
                        } break;
                        case FieldKind::ForeignKey: {
                            auto& foreign_key = field.foreign_key();
                            std::cout << "      foreign key:" << std::endl;
                            std::cout << "        col name: " << foreign_key.col;
                            std::cout << "        ref table: " << foreign_key.ref_table_name;
                            std::cout << "        ref col name: " << foreign_key.col;
                        } break;
                        default: unexpected();
                    }
                }
            } break;
            case TbStmtKind::Drop: {
                auto& drop = stmt.drop();
                std::cout << "  drop table:" << std::endl;
                std::cout << "    table name: " << drop.name;
            } break;
            case TbStmtKind::Desc: {
                auto& desc = stmt.desc();
                std::cout << "  describe table:" << std::endl;
                std::cout << "    table name: " << desc.name;
            } break;
            case TbStmtKind::InsertInto: {
                auto& insert = stmt.insert_into();
                std::cout << "  insert into table:" << std::endl;
                std::cout << "    table name: " << insert.name << std::endl;
                if (insert.columns.has_value()) {
                    auto& columns = insert.columns.get();
                    auto& values = insert.values;
                    if (columns.size() != values.size()) {
                        std::cout << "    * columns and values not match *" << std::endl;
                        std::cout << "    columns:";
                        for (auto& col : insert.columns.get()) {
                            std::cout << ' ' << col;
                        }
                        std::cout << std::endl;
                        std::cout << "    values:";
                        for (auto& val : insert.values) {
                            std::cout << ' ' << value_string(val);
                        }
                    } else {
                        std::cout << "    columns and values:" << std::endl;
                        for (size_t i = 0; i < columns.size(); i++) {
                            std::cout << "      " << columns[i] << ": " << value_string(values[i])
                                      << std::endl;
                        }
                    }
                } else {
                    std::cout << "    values:";
                    for (auto& val : insert.values) {
                        std::cout << ' ' << value_string(val);
                    }
                }
            } break;
            case TbStmtKind::DeleteFrom: {
                auto& delete_from = stmt.delete_from();
                std::cout << "  delete from table:" << std::endl;
                std::cout << "    table name: " << delete_from.name << std::endl;
                std::cout << "    where:" << std::endl;
                for (auto& cond : delete_from.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        std::cout << "      " << null.col_name << " IS "
                                  << (null.is_not_null ? "NOT " : "") << "NULL" << std::endl;
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        std::cout << "      " << o.col_name << " " << o.operator_ << " "
                                  << expression_string(o.expression);
                    }
                }
            } break;
            case TbStmtKind::Update: {
                auto& update = stmt.update();
                std::cout << "  update:" << std::endl;
                std::cout << "    table name: " << update.name << std::endl;
                std::cout << "    set:" << std::endl;
                for (auto& set : update.set) {
                    std::cout << "      " << set.col_name << ": " << value_string(set.val)
                              << std::endl;
                }
                for (auto& cond : update.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        std::cout << "      " << null.col_name << " IS "
                                  << (null.is_not_null ? "NOT " : "") << "NULL" << std::endl;
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        std::cout << "      " << o.col_name << " " << o.operator_ << " "
                                  << expression_string(o.expression);
                    }
                }
            } break;
            case TbStmtKind::Select: {
                auto& select = stmt.select();
                std::cout << "  select from table:" << std::endl;
                std::cout << "    selected cols: ";
                if (select.select.empty())
                    std::cout << "*" << std::endl;
                else {
                    for (auto& col : select.select) {
                        if (col.table_name.has_value()) std::cout << col.table_name.get() << ".";
                        std::cout << col.col_name << " ";
                    }
                    std::cout << std::endl;
                }
                std::cout << "    table list: ";
                for (auto& table : select.tables) {
                    std::cout << table << ' ';
                }
                std::cout << std::endl;
                std::cout << "    where:" << std::endl;
                for (auto& cond : select.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        std::cout << "      " << null.col_name << " IS "
                                  << (null.is_not_null ? "NOT " : "") << "NULL" << std::endl;
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        std::cout << "      " << o.col_name << " " << o.operator_ << " "
                                  << expression_string(o.expression);
                    }
                }
            } break;
            default: unexpected();
        }
    }

    void idx(idx_stmt& stmt) {}

    void alter(alter_stmt& stmt) {}

    // 遍历语法树
    void program(sql_ast& ast) {
        for (auto& stmt : ast.stmt_list) {
            switch (stmt.kind()) {
                case StmtKind::Sys: sys(stmt.sys()); break;
                case StmtKind::Db: db(stmt.db()); break;
                case StmtKind::Tb: tb(stmt.tb()); break;
                case StmtKind::Idx: idx(stmt.idx()); break;
                case StmtKind::Alter: alter(stmt.alter()); break;
                default: unexpected();
            }
        }
    }
}  // namespace Orange
