#include <algorithm>

#include "syntax.h"
#include "orange/orange.h"

static auto& cout = std::cout;
using std::endl;

namespace Orange {
    // 可能调试有用
    [[noreturn]] void unexpected() { throw std::runtime_error("unexpected error"); }

    using namespace parser;
    // 1. switch记得break
    // 2. 用auto&或const auto&而不是auto
    // 3. 包含variant的结构体带有封装了get来转换类型的方法，类型不对应会抛异常，比如本应是sys_stmt
    //    结果用.db()转成db_stmt
    // 4. 文法没有嵌套语句，现在也不确定要支持哪些嵌套语句，就先不搞

    inline std::string type_string(const data_type& type) {
        // 只有这个的kind不是方法，因为它是在产生式直接赋值的，不是variant（弄成variant=多写4个结构体+5个函数，累了
        switch (type.kind) {
            case DataTypeKind::Int:
                return type.has_value() ? "INT, " + std::to_string(type.int_value()) : "INT";
            case DataTypeKind::VarChar: return "VARCHAR, " + std::to_string(type.int_value());
            case DataTypeKind::Date: return "DATE";
            case DataTypeKind::Float: return "FLOAT";
            default: unexpected();
        }
    }
    inline std::string value_string(const data_value& value) {
        switch (value.kind()) {
            case DataValueKind::Int: return std::to_string(value.to_int());
            case DataValueKind::String: return "'" + value.to_string() + "'";
            case DataValueKind::Float: return std::to_string(value.to_float());
            case DataValueKind::Null: return "NULL";
            default: unexpected();
        }
    }
    inline std::string expression_string(const expr& e) {
        if (e.is_column()) {
            return (e.col().table_name.has_value() ? e.col().table_name.get() + "." : "") +
                   e.col().col_name;
        } else if (e.is_value()) {
            return value_string(e.value());
        } else {
            unexpected();
        }
    }

    /** 遍历语法树部分 */

    // 附加指令 比如新增调试指令会加在sys这里
    inline void sys(sys_stmt& stmt) {
        switch (stmt.kind()) {
            case SysStmtKind::ShowDb: {
                cout << "  show databases:\n    show something?\n";
                cout << all() << endl;
            } break;
            default: unexpected();
        }
    }

    inline void db(db_stmt& stmt) {
        switch (stmt.kind()) {
            case DbStmtKind::Show: {
                cout << "  show tables:" << std::endl;
                cout << "    nothing to show." << std::endl;
            } break;  // clang-format喜欢把break放在这里，为什么呢
            case DbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                cout << "  create database:" << std::endl;
                cout << "    name: " << create_stmt.name << std::endl;
                create(create_stmt.name);
            } break;
            case DbStmtKind::Drop: {
                auto& drop_stmt = stmt.drop();
                cout << "  drop database:" << std::endl;
                cout << "    name: " << drop_stmt.name << std::endl;
                drop(drop_stmt.name);
            } break;
            case DbStmtKind::Use: {
                auto& use_stmt = stmt.use();
                cout << "  use database:" << std::endl;
                cout << "    name: " << use_stmt.name << std::endl;
            } break;
            default: unexpected();
        }
    }

    inline void tb(tb_stmt& stmt) {
        switch (stmt.kind()) {
            case TbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                cout << "  create table:" << std::endl;
                cout << "    table: " << create_stmt.name << std::endl;
                cout << "    fields:" << std::endl;
                for (auto& field : create_stmt.fields) {
                    switch (field.kind()) {
                        case FieldKind::Def: {
                            auto& def = field.def();
                            cout << "      def:" << std::endl;
                            cout << "        col name: " << def.col_name << std::endl;
                            cout << "        type: " << type_string(def.type) << std::endl;
                            cout << "        is not null: " << def.is_not_null << std::endl;
                            cout << "        default value: "
                                      << (def.default_value.has_value()
                                              ? value_string(def.default_value.get())
                                              : "<none>")
                                      << std::endl;
                        } break;
                        case FieldKind::PrimaryKey: {
                            auto& primary_key = field.primary_key();
                            cout << "      primary key:" << std::endl;
                            cout << "        col list: ";
                            for (auto& col : primary_key.col_list) {
                                cout << col << ' ';
                            }
                            cout << std::endl;
                        } break;
                        case FieldKind::ForeignKey: {
                            auto& foreign_key = field.foreign_key();
                            cout << "      foreign key:" << std::endl;
                            cout << "        col name: " << foreign_key.col;
                            cout << "        ref table: " << foreign_key.ref_table_name;
                            cout << "        ref col name: " << foreign_key.col;
                        } break;
                        default: unexpected();
                    }
                }
            } break;
            case TbStmtKind::Drop: {
                auto& drop = stmt.drop();
                cout << "  drop table:" << std::endl;
                cout << "    table name: " << drop.name;
            } break;
            case TbStmtKind::Desc: {
                auto& desc = stmt.desc();
                cout << "  describe table:" << std::endl;
                cout << "    table name: " << desc.name;
            } break;
            case TbStmtKind::InsertInto: {
                auto& insert = stmt.insert_into();
                cout << "  insert into table:" << std::endl;
                cout << "    table name: " << insert.name << std::endl;
                if (insert.columns.has_value()) {
                    auto& columns = insert.columns.get();
                    auto& values = insert.values;
                    if (columns.size() != values.size()) {
                        cout << "    * columns and values not match *" << std::endl;
                        cout << "    columns:";
                        for (auto& col : insert.columns.get()) {
                            cout << ' ' << col;
                        }
                        cout << std::endl;
                        cout << "    values:";
                        for (auto& val : insert.values) {
                            cout << ' ' << value_string(val);
                        }
                    } else {
                        cout << "    columns and values:" << std::endl;
                        for (size_t i = 0; i < columns.size(); i++) {
                            cout << "      " << columns[i] << ": " << value_string(values[i])
                                      << std::endl;
                        }
                        cout << std::endl;
                    }
                } else {
                    cout << "    values:";
                    for (auto& val : insert.values) {
                        cout << ' ' << value_string(val);
                    }
                }
            } break;
            case TbStmtKind::DeleteFrom: {
                auto& delete_from = stmt.delete_from();
                cout << "  delete from table:" << std::endl;
                cout << "    table name: " << delete_from.name << std::endl;
                cout << "    where:" << std::endl;
                for (auto& cond : delete_from.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        cout << "      " << null.col_name << " IS "
                                  << (null.is_not_null ? "NOT " : "") << "NULL";
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        cout << "      " << o.col_name << " " << o.operator_ << " "
                                  << expression_string(o.expression);
                    }
                    cout << std::endl;
                }
            } break;
            case TbStmtKind::Update: {
                auto& update = stmt.update();
                cout << "  update:" << std::endl;
                cout << "    table name: " << update.name << std::endl;
                cout << "    set:" << std::endl;
                for (auto& set : update.set) {
                    cout << "      " << set.col_name << ": " << value_string(set.val)
                              << std::endl;
                }
                cout << "    where:" << std::endl;
                for (auto& cond : update.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        cout << "      " << null.col_name << " IS "
                                  << (null.is_not_null ? "NOT " : "") << "NULL";
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        cout << "      " << o.col_name << " " << o.operator_ << " "
                                  << expression_string(o.expression);
                    }
                    cout << std::endl;
                }
            } break;
            case TbStmtKind::Select: {
                auto& select = stmt.select();
                cout << "  select from table:" << std::endl;
                cout << "    selected cols: ";
                if (select.select.empty()) {  // empty时认为是select *
                    cout << "*" << std::endl;
                } else {
                    for (auto& col : select.select) {
                        if (col.table_name.has_value()) cout << col.table_name.get() << ".";
                        cout << col.col_name << " ";
                    }
                    cout << std::endl;
                }
                cout << "    table list: ";
                for (auto& table : select.tables) {
                    cout << table << ' ';
                }
                cout << std::endl;
                if (select.where.has_value()) {
                    cout << "    where:" << std::endl;
                    for (auto& cond : select.where.get()) {
                        if (cond.is_null_check()) {  // 只有两个的variant我只弄了is_xxx，要搞enum可以手动加
                            const auto& null = cond.null_check();
                            cout << "      " << null.col_name << " IS "
                                      << (null.is_not_null ? "NOT " : "") << "NULL";
                        } else if (cond.is_op()) {
                            const auto& o = cond.op();
                            cout << "      " << o.col_name << " " << o.operator_ << " "
                                      << expression_string(o.expression);
                        }
                        cout << std::endl;
                    }
                }
            } break;
            default: unexpected();
        }
    }

    // 懒了
    inline void idx(idx_stmt& stmt) {UNIMPLEMENTED}

    inline void alter(alter_stmt& stmt) {UNIMPLEMENTED}

    // 遍历语法树
    void program(sql_ast& ast) {
        for (auto& stmt : ast.stmt_list) {
            try {
                switch (stmt.kind()) {
                    case StmtKind::Sys: sys(stmt.sys()); break;
                    case StmtKind::Db: db(stmt.db()); break;
                    case StmtKind::Tb: tb(stmt.tb()); break;
                    case StmtKind::Idx: idx(stmt.idx()); break;
                    case StmtKind::Alter: alter(stmt.alter()); break;
                    default: unexpected();
                }
            } catch (OrangeException& e) {
                cout << e.what() << endl;
            }
        }
    }
}  // namespace Orange
