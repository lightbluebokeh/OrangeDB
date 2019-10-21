#pragma once

#include <defs.h>
#include <filesystem>
#include <fs/file/file.h>
#include <orange/table/filed_def.h>

class Table {
    String name;

    // table's metadata
    int record_size, record_cnt;
    std::vector<FieldDef> fields;

    void init_metadata() {
        
    }
};