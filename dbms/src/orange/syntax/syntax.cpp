#include <algorithm>
#include <map>
#include <sstream>

#include "orange/orange.h"
#include "orange/table/table.h"
#include "syntax.h"

using std::endl;

namespace Orange {
    // 可能调试有用
    [[noreturn]] void unexpected() { throw std::runtime_error("unexpected error"); }

    using namespace parser;

    static std::stringstream ss;

    inline std::string type_string(const data_type& type) {
        // 只有这个的kind不是方法，因为它是在产生式直接赋值的，不是variant（弄成variant=多写4个结构体+5个函数，累了
        switch (type.kind) {
            case orange_t::Int:
                return type.has_value() ? "INT, " + std::to_string(type.int_value()) : "INT";
            case orange_t::Varchar: return "VARCHAR, " + std::to_string(type.int_value());
            case orange_t::Date: return "DATE";
            case orange_t::Numeric:
                return "NUMERIC, " + std::to_string(type.int_value() / 40) +
                       std::to_string(type.int_value() % 40);
            default: unexpected();
        }
    }
    inline std::string value_string(const data_value& value) {
        switch (value.kind()) {
            case data_value_kind::Int: return std::to_string(value.to_int());
            case data_value_kind::String: return "'" + value.to_string() + "'";
            case data_value_kind::Float: return std::to_string(value.to_float());
            case data_value_kind::Null: return "NULL";
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
                ss << all() << endl;
            } break;
            default: unexpected();
        }
    }

    inline void db(db_stmt& stmt) {
        switch (stmt.kind()) {
            case DbStmtKind::Show: {
                ss << all_tables() << endl;
            } break;  // clang-format喜欢把break放在这里，为什么呢
            case DbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                ss << "  create database:" << std::endl;
                ss << "    name: " << create_stmt.name << std::endl;
                create(create_stmt.name);
            } break;
            case DbStmtKind::Drop: {
                auto& drop_stmt = stmt.drop();
                ss << "  drop database:" << std::endl;
                ss << "    name: " << drop_stmt.name << std::endl;
                drop(drop_stmt.name);
            } break;
            case DbStmtKind::Use: {
                auto& use_stmt = stmt.use();
                ss << "  use database:" << std::endl;
                ss << "    name: " << use_stmt.name << std::endl;
                use(use_stmt.name);
            } break;
            default: unexpected();
        }
    }

    inline void tb(tb_stmt& stmt) {
        switch (stmt.kind()) {
            case TbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                ss << "  create table:" << std::endl;
                ss << "    table: " << create_stmt.name << std::endl;
                ss << "    fields:" << std::endl;
                std::vector<Column> cols;
                std::vector<String> p_key;
                std::vector<f_key_t> f_keys;
                for (auto& field : create_stmt.fields) {
                    switch (field.kind()) {
                        case FieldKind::Def: {
                            auto& def = field.def();
                            ss << "      def:" << std::endl;
                            ss << "        col name: " << def.col_name << std::endl;
                            ss << "        type: " << type_string(def.type) << std::endl;
                            ss << "        is not null: " << def.is_not_null << std::endl;
                            ss << "        default value: "
                               << (def.default_value.has_value()
                                       ? value_string(def.default_value.get())
                                       : "<none>")
                               << std::endl;
                            cols.emplace_back(
                                def.col_name, cols.size(), def.type, !def.is_not_null,
                                def.default_value.get_value_or(ast::data_value::from_null()));
                            // cols.push_back(Column(def.col_name, def.type.kind,
                            // def.type.int_value_or(0), !def.is_not_null,
                            // def.default_value.get_value_or(data_value::null_value())));
                        } break;
                        case FieldKind::PrimaryKey: {
                            auto& primary_key = field.primary_key();
                            ss << "      primary key:" << std::endl;
                            ss << "        col list: ";
                            for (auto& col : primary_key.col_list) {
                                ss << col << ' ';
                            }
                            ss << std::endl;
                            p_key = primary_key.col_list;
                        } break;
                        case FieldKind::ForeignKey: {
                            auto& foreign_key = field.foreign_key();
                            ss << "      foreign key:" << std::endl;
                            ss << "        col name: " << foreign_key.col;
                            ss << "        ref table: " << foreign_key.ref_table_name;
                            ss << "        ref col name: " << foreign_key.col << endl;

                            f_keys.push_back(f_key_t(foreign_key.col, foreign_key.ref_table_name,
                                                     {foreign_key.col},
                                                     {foreign_key.ref_col_name}));
                        } break;
                        default: unexpected();
                    }
                }
                SavedTable::create(create_stmt.name, cols, p_key, f_keys);
            } break;
            case TbStmtKind::Drop: {
                auto& drop = stmt.drop();
                ss << "  drop table:" << std::endl;
                ss << "    table name: " << drop.name << endl;
                SavedTable::drop(drop.name);
            } break;
            case TbStmtKind::Desc: {
                auto& desc = stmt.desc();
                ss << "  describe table:" << std::endl;
                ss << "    table name: " << desc.name << endl;
                ss << SavedTable::get(desc.name)->description() << endl;
            } break;
            case TbStmtKind::InsertInto: {
                auto& insert = stmt.insert_into();
                ss << "  insert into table:" << std::endl;
                ss << "    table name: " << insert.name << std::endl;
                auto table = SavedTable::get(insert.name);
                if (insert.columns.has_value()) {
                    auto& columns = insert.columns.get();
                    auto& values_list = insert.values_list;
                    [&] {
                        for (auto& values : values_list) {
                            if (columns.size() != values.size()) {
                                ss << "    * columns and values not match *" << std::endl;
                                ss << "    columns:";
                                for (auto& col : insert.columns.get()) {
                                    ss << ' ' << col;
                                }
                                ss << std::endl;
                                ss << "    values:";
                                for (auto& values : insert.values_list) {  // 这个循环好像重复了？
                                    for (auto& val : values) {
                                        ss << ' ' << value_string(val) << endl;
                                    }
                                }
                                return;
                            }
                        }
                        for (const auto& column : columns) {
                            ss << "      " << column << " ";
                        }
                        ss << endl;
                        for (auto& values : values_list) {
                            std::vector<std::pair<String, byte_arr_t>> data;
                            for (auto& val : values) {
                                ss << "       " << value_string(val);
                                // data.push_back(std::make_pair(columns[i],
                                // Orange::to_bytes(val)));
                            }
                            // table->insert(data);
                            ss << endl;
                        }
                        table->insert(insert.columns.get(), insert.values_list);
                    }();
                } else {
                    for (auto& values : insert.values_list) {
                        ss << "    values:";
                        for (auto& val : values) {
                            ss << ' ' << value_string(val);
                        }
                        ss << endl;
                    }
                    table->insert(insert.values_list);
                }
            } break;
            case TbStmtKind::DeleteFrom: {
                auto& delete_from = stmt.delete_from();
                ss << "  delete from table:" << std::endl;
                ss << "    table name: " << delete_from.name << std::endl;
                ss << "    where:" << std::endl;
                for (auto& cond : delete_from.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        ss << "      " << null.col_name << " IS "
                           << (null.is_not_null ? "NOT " : "") << "NULL";

                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        ss << "      " << o.col_name << " " << o.operator_ << " "
                           << expression_string(o.expression);
                    }
                    ss << std::endl;
                }
                SavedTable::get(delete_from.name)->delete_where(delete_from.where);
            } break;
            case TbStmtKind::Update: {
                auto& update = stmt.update();
                ss << "  update:" << std::endl;
                ss << "    table name: " << update.name << std::endl;
                ss << "    set:" << std::endl;
                for (auto& set : update.set) {
                    ss << "      " << set.col_name << ": " << value_string(set.val) << std::endl;
                }
                ss << "    where:" << std::endl;
                for (auto& cond : update.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        ss << "      " << null.col_name << " IS "
                           << (null.is_not_null ? "NOT " : "") << "NULL";
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        ss << "      " << o.col_name << " " << o.operator_ << " "
                           << expression_string(o.expression);
                    }
                    ss << std::endl;
                }
            } break;
            case TbStmtKind::Select: {
                auto& select = stmt.select();
                ss << "  select from table:" << std::endl;
                std::map<String, SavedTable*> tables;
                ss << "    table list: ";
                for (auto& table : select.tables) {
                    ss << table << ' ';
                    tables[table] = SavedTable::get(table);
                }
                ss << std::endl;
                ss << "    selected cols: ";
                if (select.select.empty()) {  // empty时认为是select *
                    ss << "*" << std::endl;
                } else {
                    for (auto& col : select.select) {
                        if (col.table_name.has_value()) {
                            auto name = col.table_name.get();
                            orange_check(tables.count(name), "unknown table name: `" + name + "`");
                            ss << name << ".";
                        }
                        ss << col.col_name << " ";
                    }
                    ss << std::endl;
                }
                if (select.where.has_value()) {
                    ss << "    where:" << std::endl;
                    for (auto& cond : select.where.get()) {
                        if (cond.is_null_check()) {  // 只有两个的variant我只弄了is_xxx，要搞enum可以手动加
                            const auto& null = cond.null_check();
                            ss << "      " << null.col_name << " IS "
                               << (null.is_not_null ? "NOT " : "") << "NULL";
                        } else if (cond.is_op()) {
                            const auto& o = cond.op();
                            ss << "      " << o.col_name << " " << o.operator_ << " "
                               << expression_string(o.expression);
                        }
                        ss << std::endl;
                    }
                }
                if (select.select.empty()) {
                    if (tables.size() == 1) {
                        auto table = tables.begin()->second;
                        ss << table->select_star(select.where.get_value_or({})) << endl;
                    } else {
                        ORANGE_UNIMPL
                    }
                } else {
                    if (tables.size() == 1) {
                        auto table = tables.begin()->second;
                        std::vector<String> names;
                        for (const auto& col : select.select) {
                            names.push_back(col.col_name);
                        }
                        ss << table->select(names, select.where.get_value_or({})) << endl;
                    } else {
                        ORANGE_UNIMPL
                    }
                }
            } break;
            default: unexpected();
        }
    }

    // 懒了
    inline void idx(idx_stmt& stmt) {
        switch (stmt.kind()) {
            case IdxStmtKind::AlterAdd: {
                ORANGE_UNIMPL
            } break;
            case IdxStmtKind::AlterDrop: {
                ORANGE_UNIMPL
            } break;
            case IdxStmtKind::Create: {
                // auto &create = stmt.create();
                // SavedTable::get(create.tb_name)->create_index(create.col_list, create.idx_name);
            } break;
            case IdxStmtKind::Drop: {
                ORANGE_UNIMPL
            } break;
        }
    }

    inline void alter(alter_stmt& stmt) {
        // 乱写
        ss << stmt.add_field().table_name << endl;
    }

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
            }
            catch (OrangeException& e) {
                ss << RED << "FAILED: " << RESET << e.what() << endl;
            }
        }
        std::cout << ss.str();
    }
}  // namespace Orange
