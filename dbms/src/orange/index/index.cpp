#include "orange/table/table.h"
#include "orange/index/index.h"

Index* Index::create(SavedTable& table, const std::vector<Column>& cols, const String& name) {
    auto path = table.index_root() + name + "/";
    fs::create_directory(path);
    auto index = new Index(table, table.index_root() + name + "/", Column::key_size_sum(cols), cols);
}

