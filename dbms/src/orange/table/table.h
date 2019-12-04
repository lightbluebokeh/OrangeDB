#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>

#include <defs.h>
#include <fs/file/file.h>
#include <orange/index/index.h>
#include <orange/orange.h>
#include <orange/table/column.h>
#include <orange/table/key.h>
#include <orange/table/table_base.h>
#include <utils/id_pool.h>

// 数据库中的表
class SavedTable : public Table {
private:
    int id;
    String name;
    IdPool<rid_t> rid_pool;

    SavedTable(int id, const String& name) : id(id), name(name), rid_pool(pool_name()) {}

    // metadata(还包括 Table::cols)
    rid_t rec_cnt;
    std::vector<String> p_key;
    std::vector<f_key_t> f_keys;

    // map index by their name
    std::unordered_map<String, Index*> indexes;

    String root() { return get_root(name); }
    String metadata_name() { return root() + "metadata.table"; }
    String pool_name() { return root() + "rid.pl"; }
    String data_root() { return root() + "data/"; }
    String col_prefix(const String& col_name) { return data_root() + col_name; }
    String data_name(const String& col_name) { return col_prefix(col_name) + ".data"; };
    String index_root() { return root() + "index/"; }

    void write_metadata() {
        std::ofstream ofs(metadata_name());
        print(ofs, ' ', cols, rec_cnt, p_key, f_keys);
        ofs << ' ' << indexes.size();
        for (auto &[idx_name, idx]: indexes) {
            const auto& cols = idx->get_cols();
            ofs << ' ' << idx_name << ' ' << cols.size();
            for (const auto &col: cols) {
                ofs << ' ' << col.get_id();
            }
        }
    }
    void read_metadata() {
        std::ifstream ifs(metadata_name());
        ifs >> cols >> rec_cnt >> p_key >> f_keys;
        int index_num;
        ifs >> index_num;
        while (index_num--) {
            String idx_name;
            std::vector<Column> idx_cols;
            int col_num;
            ifs >> idx_name >> col_num;
            while (col_num--) {
                int col_id;
                ifs >> col_id;
                idx_cols.push_back(cols[col_id]);
            }
            auto index = new Index(*this, idx_name, cols);;
            index->load();
            indexes[idx_name] = index;
        }
    }

    static SavedTable* tables[MAX_TBL_NUM];
    static SavedTable* new_table(const String& name) {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (!tables[i]) {
                tables[i] = new SavedTable(i, name);
                tables[i]->id = i;
                return tables[i];
            }
        }
        throw OrangeException("no more tables plz >_<");
    }

    static void free_table(SavedTable* table) {
        if (table == nullptr) return;
        tables[table->id] = nullptr;
        delete table;
    }

    static void check_db() { orange_ensure(Orange::using_db(), "use some database first"); }
    
    void on_create(const std::vector<Column>& cols, const std::vector<String>& p_key, const std::vector<f_key_t>& f_keys) {
        this->cols = cols;
        this->p_key = p_key;
        this->f_keys = f_keys;
        this->rec_cnt = 0;
        File::create(metadata_name());
        write_metadata();
        rid_pool.init();
        fs::create_directory(data_root());
        fs::create_directory(index_root());

        // primary key
        std::vector<Column> p_cols;
        for (const auto &name: p_key) {
            p_cols.push_back(get_col(name));
        }
        auto index = new Index(*this, "", cols);    // primary key 没有名字
        indexes[""] = index;

        // foreign key
        ORANGE_UNIMPL
    }

    static String get_root(const String& name) { return name + "/"; }

    // 在打开的表里面找
    static SavedTable* get_opened(const String& name) {
        for (int i = 0; i < MAX_TBL_NUM; i++) {
            if (tables[i] && tables[i]->name == name) {
                return tables[i];
            }
        }
        return nullptr;
    }
protected:
    std::vector<rid_t> all() const override { return rid_pool.all(); }

    byte_arr_t get_field(rid_t rid, int col_id) const override { return this->indices[col_id].get_val(rid); }
public:
    static bool create(const String& name, const std::vector<Column>& cols, const std::vector<String>& p_key,
                       const std::vector<f_key_t>& f_keys) {
        check_db();
        orange_ensure(!fs::exists(get_root(name)), "table `" + name + "` exists");
        std::error_code e;
        if (!fs::create_directory(get_root(name), e)) throw e.message();
        auto table = new_table(name);
        table->on_create(cols, p_key, f_keys);
        return 1;
    }

    static SavedTable* get(const String& name) {
        check_db();
        orange_ensure(fs::exists(get_root(name)), "table `" + name + "` does not exists");
        auto table = get_opened(name);
        if (!table) {
            table = new_table(name);
            table->read_metadata();
            table->rid_pool.load();
        }
        return table;
    }

    TmpTable description() {
        TmpTable ret;
        ret.cols.push_back(Column("Name"));
        ret.cols.push_back(Column("null"));
        ret.cols.push_back(Column("type"));
        ret.recs.resize(cols.size());
        for (unsigned i = 0; i < cols.size(); i++) {
            ret.recs[i].push_back(Orange::to_bytes(cols[i].get_name()));
            ret.recs[i].push_back(Orange::to_bytes(cols[i].nullable ? "nullable" : "not null"));
            ret.recs[i].push_back(Orange::to_bytes(cols[i].type_string()));
        }
        return ret;
    }

    // 关闭表，写回缓存，数据在 index 的析构中写回
    bool close() {
        orange_ensure(this == tables[id], "this is magic");
        write_metadata();
        free_table(this);
        return 1;
    }

    static bool drop(const String& name) {
        auto table = get_opened(name);
        if (table) table->close();
        return fs::remove_all(name);
    }

    // 一般是换数据库的时候调用这个
    static void close_all() {
        for (auto &table : tables) {
            if (table) {
                orange_ensure(table->close(), "close table failed");
                delete table;
                table = nullptr;
            }
        }
    }

    // 按列插入，会先尝试把输入的值补全
    void insert(const std::vector<std::pair<String, ast::data_value>>& values) {
        std::unordered_map<String, ast::data_value> map;
        for (auto [name, val]: values) {
            orange_ensure(has_col(name), "insertion failed: unknown column name");
            map[name] = val;
        }
        std::vector<ast::data_value> values_all;
        for (auto &col: cols) {
            auto it = map.find(col.get_name());
            if (it == map.end()) {
                values_all.push_back(col.get_dft());
            } else {
                values_all.push_back(it->second);
            }
        }
        insert(values_all);
    }

    // 直接插入完整的一条
    void insert(std::vector<ast::data_value> values) {
        // 检查参数个数是否等于列数
        orange_ensure(values.size() == cols.size(), "insertion failed: expected " + std::to_string(cols.size()) + " values while " + std::to_string(values.size()) + "given");
        rec_t rec;
        for (unsigned i = 0; i < cols.size(); i++) {

        }
        // for (auto &value: values) rec.push_back(Orange::value_to_bytes(value, col_size));
        // insert_internal(values);
    }

    std::vector<rid_t> single_where(const Orange::parser::single_where& where, rid_t lim) const override {
        std::vector<rid_t> ret;
        if(where.is_null_check()) {
            auto &null = where.null_check();
            ret = indices[get_col_id(null.col_name)].get_rids_null(null.is_not_null, lim);
        } else if (where.is_op()) {
            auto &op = where.op();
            if (op.expression.is_value()) {
                ret = indices[get_col_id(op.col_name)].get_rids_value(op.operator_, op.expression.value(), lim);
            } else if (op.expression.is_column()) { // 只会暴力
                auto &col = op.expression.col();
                orange_ensure(!col.table_name.has_value(), "no table name in where clause");
                auto &idx1 = indices[get_col_id(op.col_name)], &idx2 = indices[get_col_id(col.col_name)];
                auto kind1 = find_col(op.col_name)->get_datatype(), kind2 = find_col(col.col_name)->get_datatype();
                for (auto rid: all()) {
                    if (ret.size() >= lim) break;
                    if (Orange::cmp(idx1.get_val(rid), kind1, op.operator_, idx2.get_val(rid), kind2)) {
                        ret.push_back(rid);
                    }
                }
            } else {
                ORANGE_UNREACHABLE
                ret = {};
            }
        } else {
            ORANGE_UNREACHABLE
            ret = {};
        }
        for (auto rid: ret) {
            orange_assert(check_where(where, rid), "where failed");
        }
        return ret;
    }

    void remove(const std::vector<rid_t>& rids) {
        for (auto &index: indices) {
            for (auto rid: rids) {
                index.remove(rid);
            }
        }
        for (auto rid: rids) {
            rid_pool.free_id(rid);
        }
    }

    TmpTable select(std::vector<String> names, const std::vector<rid_t>& rids) const override {
        TmpTable ret;
        ret.recs.resize(rids.size());
        std::vector<int> col_ids;
        for (auto name : names) {
            auto col_id = get_col_id(name);
            ret.cols.push_back(cols[col_id]);
            auto vals = indices[col_id].get_vals(rids);
            for (unsigned i = 0; i < rids.size(); i++) {
                ret.recs[i].push_back(vals[i]);
            }
        }
        return ret;
    }

    void update(const String& col_name, std::function<byte_arr_t(byte_arr_t)> expr, const std::vector<rid_t>& rids) {
        int col_id = get_col_id(col_name);
        auto &idx = indices[col_id];
        auto &col = cols[col_id];
        for (auto rid: rids) {
            auto new_val = expr(idx.get_val(rid));
            orange_ensure(col.test(new_val), "update failed: new value fails constraint");
            idx.update(new_val, rid);
        }
    }

    // void create_index(const String& col_name) {
    //     auto it = find_col(col_name);
    //     orange_ensure(it != cols.end(), "create index failed: unknown column name");
    //     auto col_id = it - cols.begin();
    //     indices[col_id].turn_on();
    // }

    void create_index(const String& idx_name, const std::vector<String>& col_names) {
        orange_ensure(!indexes.count(idx_name), "already has index named `" + idx_name + "`");
        auto index = new Index(*this, idx_name, get_cols(col_names));
        ORANGE_UNIMPL
    }
    void drop_index(const String& idx_name) {
        orange_ensure(indexes.count(idx_name), "index `" + name + "` does not exists");
        indexes[idx_name]->drop();
    }

    // void drop_index(const String& col_name) {
    //     auto it = find_col(col_name);
    //     orange_ensure(it != cols.end(), "drop index failed: unknown column name");
    //     auto col_id = it - cols.begin();
    //     indices[col_id].turn_off();
    // }

    friend class Index;
};
