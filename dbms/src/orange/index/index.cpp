#include "orange/table/table.h"
#include "orange/index/b_tree.h"

fs::path Index::get_path() const {
    return table.index_root() / name;
}

Index* Index::create(SavedTable& table, const String& name, const std::vector<Column>& cols, bool primary, bool unique) {
    auto index = new Index(table, name);
    fs::create_directory(index->get_path());
    index->cols = cols;
    index->key_size = Column::key_size_sum(cols);
    index->primary = primary;
    index->unique = unique;
    index->tree = new BTree(*index, index->key_size, index->get_path());
    index->tree->init();
    for (auto rid: table.all()) {
        index->insert(table.get_raws(cols, rid), rid);
    }
    return index;
}

Index* Index::load(SavedTable& table, const String& name) {
    auto index = new Index(table, name);
    index->read_info();
    index->key_size = Column::key_size_sum(index->cols);
    index->tree = new BTree(*index, index->key_size, index->get_path());
    index->tree->load();
    return index;
}

// 获取多值索引中的第 i 项的真实值
byte_arr_t Index::restore(const_bytes_t k_raw, int i) const {
    int offset = 0;
    // 计算偏移
    for (int j = 0; j < i; j++) offset += cols[j].get_key_size();

    return table.col_data[table.get_col_id(cols[i].get_name())]->restore(k_raw + offset);
}
std::vector<byte_arr_t> Index::restore(const_bytes_t k_raw) const {
    std::vector<byte_arr_t> ret;
    for (unsigned i = 0, offset = 0; i < cols.size(); i++) {
        auto &col = cols[i];
        ret.push_back(table.col_data[table.get_col_id(col.get_name())]->restore(k_raw + offset));
        offset += col.get_key_size();
    }
    return ret;
}
