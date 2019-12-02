#include <orange/table/table.h>
#include <orange/index/index.h>

std::vector<rid_t> Index::get_all() const { return table.all(); }

