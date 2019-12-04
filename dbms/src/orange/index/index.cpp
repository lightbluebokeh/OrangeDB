#include <orange/table/table.h>
#include <orange/index/index.h>

Index::Index(SavedTable& table, const String& name, const std::vector<Column>& cols) 
    : table(table), name(name), cols(cols), tree(*this, Column::key_size_sum(cols), table.index_root() + name) {}
