#include <defs.h>
#include <exception>
#include <fs/file/file.h>
#include <orange/table/table.h>

File* File::files[MAX_FILE_NUM];
SavedTable* SavedTable::tables[MAX_TBL_NUM];

void ensure(bool cond, const String& msg) {
    if (cond == 0) {
        // std::cerr << RED << "failed: " << RESET << msg << std::endl;
        throw OrangeException(msg);
    }
}

std::vector<Column>::iterator Table::find_col(const String& col_name) {
    auto it = cols.begin();
    while (it != cols.end() && it->get_name() != col_name) it++;
    ensure(it != cols.end(), "unknown column name: `" + col_name + "`");
    return it;
}

std::vector<rid_t> TmpTable::where(const String& col_name, pred_t pred, rid_t lim) {
    std::vector<rid_t> ret;
    auto it = find_col(col_name);
    int col_id = it - cols.begin();
    for (unsigned i = 0; i < recs.size(); i++) {
        if (pred.test(recs[i][col_id], it->get_datatype())) ret.push_back(i);
    }
    return ret;
}

TmpTable TmpTable::select(std::vector<String> names, const std::vector<rid_t>& rids) {
    TmpTable ret;
    ret.recs.resize(rids.size());
    for (auto name: names) {
        auto col_id = find_col(name) - cols.begin();
        ret.cols.push_back(cols[col_id]);
        for (auto rid: rids) {
            ret.recs[rid].push_back(recs[rid][col_id]);
        }
    }
    return ret;
}