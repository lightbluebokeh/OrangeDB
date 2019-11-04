#include <orange/table/table.h>
#include <orange/index/index.h>

rid_t Index::get_tot() { return table.rid_pool.get_tot(); }