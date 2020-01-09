#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "orange/orange.h"
#include "orange/table/table.h"
#include "syntax.h"

using std::endl;

constexpr auto a = std::make_pair(1, 2), b = std::make_pair(2, 3);
static_assert(a < b);

namespace Orange {
    [[noreturn]] void unexpected() { throw std::runtime_error("unexpected error"); }

    using namespace parser;

    static std::stringstream ss_debug;

    inline std::string type_string(const data_type& type) {
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

    /** 遍历语法树 */

    inline result_t sys(sys_stmt& stmt) {
        switch (stmt.kind()) {
            case SysStmtKind::ShowDb: {
                return {TmpTable::from_strings("tables", all())};
            }
            default: unexpected();
        }
    }

    inline result_t db(db_stmt& stmt) {
        switch (stmt.kind()) {
            case DbStmtKind::Show: {
                return result_t(TmpTable::from_strings("tables", all_tables()));
            }
            case DbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                ss_debug << "  create database:" << std::endl;
                ss_debug << "    name: " << create_stmt.name << std::endl;
                create(create_stmt.name);
                return {{}};
            }
            case DbStmtKind::Drop: {
                auto& drop_stmt = stmt.drop();
                ss_debug << "  drop database:" << std::endl;
                ss_debug << "    name: " << drop_stmt.name << std::endl;
                drop(drop_stmt.name);
                return {{}};
            }
            case DbStmtKind::Use: {
                auto& use_stmt = stmt.use();
                ss_debug << "  use database:" << std::endl;
                ss_debug << "    name: " << use_stmt.name << std::endl;
                use(use_stmt.name);
                return {{}};
            }
            default: unexpected();
        }
    }

    inline result_t tb(tb_stmt& stmt) {
        switch (stmt.kind()) {
            case TbStmtKind::Create: {
                auto& create_stmt = stmt.create();
                ss_debug << "  create table:" << std::endl;
                ss_debug << "    table: " << create_stmt.name << std::endl;
                ss_debug << "    fields:" << std::endl;
                std::vector<Column> cols;
                std::vector<String> p_key;
                std::vector<f_key_t> f_keys;
                for (const auto& field : create_stmt.fields) {
                    switch (field.kind()) {
                        case FieldKind::Def: {
                            auto& def = field.def();
                            ss_debug << "      def:" << std::endl;
                            ss_debug << "        col name: " << def.col_name << std::endl;
                            ss_debug << "        type: " << type_string(def.type) << std::endl;
                            ss_debug << "        is not null: " << def.is_not_null << std::endl;
                            ss_debug << "        default value: "
                                     << (def.default_value.has_value()
                                             ? value_string(def.default_value.get())
                                             : "<none>")
                                     << std::endl;
                            cols.emplace_back(Column::from_def(def));
                        } break;
                        case FieldKind::PrimaryKey: {
                            auto& primary_key = field.primary_key();
                            ss_debug << "      primary key:" << std::endl;
                            ss_debug << "        col list: ";
                            for (auto& col : primary_key.col_list) {
                                ss_debug << col << ' ';
                            }
                            ss_debug << std::endl;
                            p_key = primary_key.col_list;
                        } break;
                        case FieldKind::ForeignKey: {
                            auto& foreign_key = field.foreign_key();
                            ss_debug << "      foreign key:" << std::endl;
                            ss_debug << "        col name: " << foreign_key.col;
                            ss_debug << "        ref table: " << foreign_key.ref_table_name;
                            ss_debug << "        ref col name: " << foreign_key.col << endl;

                            f_keys.push_back(f_key_t(foreign_key.col, foreign_key.ref_table_name,
                                                     {foreign_key.col},
                                                     {foreign_key.ref_col_name}));
                        } break;
                        default: unexpected();
                    }
                }
                SavedTable::create(create_stmt.name, cols, p_key, f_keys);
                return {{}};
            }
            case TbStmtKind::Drop: {
                auto& drop = stmt.drop();
                ss_debug << "  drop table:" << std::endl;
                ss_debug << "    table name: " << drop.name << endl;
                SavedTable::drop(drop.name);
                return {{}};
            }
            case TbStmtKind::Desc: {
                auto& desc = stmt.desc();
                ss_debug << "  describe table:" << std::endl;
                ss_debug << "    table name: " << desc.name << endl;
                return {SavedTable::get(desc.name)->description()};
            }
            case TbStmtKind::InsertInto: {
                auto& insert = stmt.insert_into();
                ss_debug << "  insert into table:" << std::endl;
                ss_debug << "    table name: " << insert.name << std::endl;
                auto table = SavedTable::get(insert.name);
                if (insert.columns.has_value()) {
                    auto& columns = insert.columns.get();
                    auto& values_list = insert.values_list;
                    [&] {
                        for (auto& values : values_list) {
                            if (columns.size() != values.size()) {
                                ss_debug << "    * columns and values not match *" << std::endl;
                                ss_debug << "    columns:";
                                for (auto& col : insert.columns.get()) {
                                    ss_debug << ' ' << col;
                                }
                                ss_debug << std::endl;
                                ss_debug << "    values:";
                                for (auto& val : values) {
                                    ss_debug << ' ' << value_string(val) << endl;
                                }
                                return;
                            }
                        }
                        for (const auto& column : columns) {
                            ss_debug << "      " << column << " ";
                        }
                        ss_debug << endl;
                        for (auto& values : values_list) {
                            std::vector<std::pair<String, byte_arr_t>> data;
                            for (auto& val : values) {
                                ss_debug << "       " << value_string(val);
                                // data.push_back(std::make_pair(columns[i],
                                // Orange::to_bytes(val)));
                            }
                            // table->insert(data);
                            ss_debug << endl;
                        }
                        table->insert(insert.columns.get(), insert.values_list);
                    }();
                } else {
                    for (auto& values : insert.values_list) {
                        ss_debug << "    values:";
                        for (auto& val : values) {
                            ss_debug << ' ' << value_string(val);
                        }
                        ss_debug << endl;
                    }
                    table->insert(insert.values_list);
                }
                return {{}};
            }
            case TbStmtKind::DeleteFrom: {
                auto& delete_from = stmt.delete_from();
                ss_debug << "  delete from table:" << std::endl;
                ss_debug << "    table name: " << delete_from.name << std::endl;
                ss_debug << "    where:" << std::endl;
                for (auto& cond : delete_from.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        ss_debug << "      " << null.col_name << " IS "
                                 << (null.is_not_null ? "NOT " : "") << "NULL";

                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        ss_debug << "      " << o.col_name << " " << o.operator_ << " "
                                 << expression_string(o.expression);
                    }
                    ss_debug << std::endl;
                }
                SavedTable::get(delete_from.name)->delete_where(delete_from.where);
                return {{}};
            }
            case TbStmtKind::Update: {
                auto& update = stmt.update();
                ss_debug << "  update:" << std::endl;
                ss_debug << "    table name: " << update.name << std::endl;
                ss_debug << "    set:" << std::endl;
                for (auto& set : update.set) {
                    ss_debug << "      " << set.col_name << ": " << value_string(set.val)
                             << std::endl;
                }
                ss_debug << "    where:" << std::endl;
                for (auto& cond : update.where) {
                    if (cond.is_null_check()) {
                        const auto& null = cond.null_check();
                        ss_debug << "      " << null.col_name << " IS "
                                 << (null.is_not_null ? "NOT " : "") << "NULL";
                    } else if (cond.is_op()) {
                        const auto& o = cond.op();
                        ss_debug << "      " << o.col_name << " " << o.operator_ << " "
                                 << expression_string(o.expression);
                    }
                    ss_debug << std::endl;
                }
                SavedTable::get(update.name)->update_where(update.set, update.where);
                return {{}};
            }
            case TbStmtKind::Select: {
                auto& select = stmt.select();
                ss_debug << "  select from table:" << std::endl;
                std::map<String, SavedTable*> tables;
                ss_debug << "    table list: ";
                for (auto& table : select.tables) {
                    ss_debug << table << ' ';
                    tables[table] = SavedTable::get(table);
                }
                ss_debug << std::endl;
                ss_debug << "    selected cols: ";
                if (select.select.empty()) {  // empty时认为是select *
                    ss_debug << "*" << std::endl;
                } else {
                    for (auto& col : select.select) {
                        if (col.table_name.has_value()) {
                            auto name = col.table_name.get();
                            orange_check(tables.count(name), "unknown table name: `" + name + "`");
                            ss_debug << name << ".";
                        }
                        ss_debug << col.col_name << " ";
                    }
                    ss_debug << std::endl;
                }
                if (select.where.has_value()) {
                    ss_debug << "    where:" << std::endl;
                    for (auto& cond : select.where.get()) {
                        if (cond.is_null_check()) {
                            const auto& null = cond.null_check();
                            ss_debug << "      " << null.col_name << " IS "
                                     << (null.is_not_null ? "NOT " : "") << "NULL";
                        } else if (cond.is_op()) {
                            const auto& o = cond.op();
                            ss_debug << "      " << o.col_name << " " << o.operator_ << " "
                                     << expression_string(o.expression);
                        }
                        ss_debug << std::endl;
                    }
                }
                if (select.select.empty()) {
                    if (tables.size() == 1) {
                        auto table = tables.begin()->second;
                        return {table->select_star(select.where.get_value_or({}))};
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
                        return {table->select(names, select.where.get_value_or({}))};
                    } else {
                        ORANGE_UNIMPL
                    }
                }
            } break;
            default: unexpected();
        }
    }

    inline result_t idx(idx_stmt& stmt) {
        switch (stmt.kind()) {
            case IdxStmtKind::AlterAdd: {
                auto alter_add = stmt.alter_add();
                SavedTable::get(alter_add.tb_name)->create_index(alter_add.idx_name, alter_add.col_list, 0, 0);
            } break;
            case IdxStmtKind::AlterDrop: {
                auto alter_drop = stmt.alter_drop();
                SavedTable::get(alter_drop.tb_name)->drop_index(alter_drop.idx_name);
            } break;
            case IdxStmtKind::Create: {
                auto& create = stmt.create();
                ss_debug << "  create index" << endl;
                ss_debug << "   table: " + create.tb_name << endl;
                ss_debug << "   index name: " + create.idx_name << endl;
                ss_debug << "   columns: " << endl;
                for (auto& col : create.col_list) ss_debug << "       " << col << endl;
                SavedTable::get(create.tb_name)
                    ->create_index(create.idx_name, create.col_list, 0, 0);  // unique 不知道
            } break;
            case IdxStmtKind::Drop: {
                auto& drop = stmt.drop();
                ss_debug << "  drop index" << endl;
                ss_debug << "   index name: " << drop.name << endl;
                for (const auto& tbl_name : all_tables()) {
                    auto table = SavedTable::get(tbl_name);
                    if (table->has_index(drop.name)) table->drop_index(drop.name);
                }
            } break;
        }
        return {{}};
    }

    inline result_t alter(alter_stmt& stmt) {
        switch (stmt.kind()) {
            case AlterStmtKind::AddField: {
                const auto& add_field = stmt.add_field();
                const std::string& table_name = add_field.table_name;
                auto table = SavedTable::get(table_name);
                const single_field& new_field = add_field.new_field;
                if (new_field.kind() == FieldKind::Def) {
                    auto &def = new_field.def();
                    auto col = Column::from_def(def);
                    table->add_col(col);
                } else {
                    unexpected();
                }
            } break;
            case AlterStmtKind::DropCol: {
                const auto& drop_col = stmt.drop_col();
                const std::string& table_name = drop_col.table_name;
                const std::string& col_name = drop_col.col_name;
                auto table = SavedTable::get(table_name);
                table->drop_col(col_name);
            } break;
            case AlterStmtKind::ChangeCol: {
                const auto& change_col = stmt.change_col();
                const std::string& table_name = change_col.table_name;
                const std::string& col_name = change_col.col_name;
                const single_field& new_field = change_col.new_field;

                auto table = SavedTable::get(table_name);
                if (new_field.kind() == FieldKind::Def) {
                    table->change_col(col_name, new_field.def());
                } else {
                    unexpected();
                }
            } break;
            case AlterStmtKind::RenameTb: {
                const auto& rename_table = stmt.rename_tb();
                const std::string& table_name = rename_table.table_name;
                const std::string& new_tb_name = rename_table.new_tb_name;

                SavedTable::rename(table_name, new_tb_name);
            } break;
            case AlterStmtKind::AddPrimaryKey: {
                const auto& add_primary_key = stmt.add_primary_key();
                const std::string& table_name = add_primary_key.table_name;
                const column_list& col_list = add_primary_key.col_list;
                ss_debug << "  add primary key: " << endl;
                ss_debug << "   table: " + add_primary_key.table_name << endl;
                ss_debug << String("   primary key name: ") + PRIMARY_KEY_NAME << endl;
                ss_debug << "   columns: " << endl;
                for (const std::string& col : col_list) {
                    ss_debug << "       " << col << endl;
                }

                auto table = SavedTable::get(table_name);
                table->add_p_key(PRIMARY_KEY_NAME, col_list);
            } break;
            case AlterStmtKind::AddConstraintPrimaryKey: {
                const auto& add_constraint_primary_key = stmt.add_constraint_primary_key();
                const std::string& table_name = add_constraint_primary_key.table_name;
                const std::string& pk_name = add_constraint_primary_key.pk_name;
                const column_list& col_list = add_constraint_primary_key.col_list;
                ss_debug << "  add primary key: " << endl;
                ss_debug << "   table: " + add_constraint_primary_key.table_name << endl;
                ss_debug << String("   primary key name: ") + add_constraint_primary_key.pk_name << endl;
                ss_debug << "   columns: " << endl;
                for (const std::string& col : col_list) {
                    ss_debug << "       " << col << endl;
                }

                auto table = SavedTable::get(table_name);
                table->add_p_key(pk_name, col_list);
            } break;
            case AlterStmtKind::DropPrimaryKey: {
                const auto& drop_primary_key = stmt.drop_primary_key();
                const std::string& table_name = drop_primary_key.table_name;
                const boost::optional<std::string>& pk_name = drop_primary_key.pk_name;

                auto table = SavedTable::get(table_name);
                table->drop_p_key(pk_name.get_value_or(PRIMARY_KEY_NAME));
            } break;
            case AlterStmtKind::AddConstraintForeignKey: {
                const auto& add_constraint_foreign_key = stmt.add_constraint_foreign_key();
                const std::string& table_name = add_constraint_foreign_key.table_name;
                const std::string& fk_name = add_constraint_foreign_key.fk_name;
                const column_list& col_list = add_constraint_foreign_key.col_list;
                const std::string& ref_tb_name = add_constraint_foreign_key.ref_tb_name;
                const column_list& ref_col_list = add_constraint_foreign_key.ref_col_list;

                auto table = SavedTable::get(table_name);
                table->add_f_key(f_key_t{fk_name, ref_tb_name, col_list, ref_col_list});
            } break;
            case AlterStmtKind::DropForeignKey: {
                const auto& drop_foreign_key = stmt.drop_foreign_key();
                const std::string& table_name = drop_foreign_key.table_name;
                const std::string& fk_name = drop_foreign_key.fk_name;

                auto table = SavedTable::get(table_name);
                table->drop_f_key(fk_name);
            } break;
            default: unexpected();
        }
        return {{}};
    }

    result_t stmt(ast::sql_stmt& stmt) {
        try {
            switch (stmt.kind()) {
                case StmtKind::Sys: return sys(stmt.sys());
                case StmtKind::Db: return db(stmt.db());
                case StmtKind::Tb: return tb(stmt.tb());
                case StmtKind::Idx: return idx(stmt.idx());
                case StmtKind::Alter: return alter(stmt.alter());
                default: unexpected();
            }
        }
        catch (OrangeException& e) {
            return {e.what()};
        }
    }

    // 遍历语法树
    // 不抛异常
    std::vector<result_t> program(sql_ast& ast) noexcept {
        std::vector<result_t> ret;
        std::stringstream tmp;  // new 一对象
        ss_debug.swap(tmp);
        for (auto& stmt : ast.stmt_list) {
            ss_debug.clear();
            ret.push_back(Orange::stmt(stmt));
#ifdef DEBUG
            std::cerr << ss_debug.str() << std::endl;
#endif
        }
        return ret;
    }
}  // namespace Orange
