#include <exception>

#include "defs.h"
#include "fs/file/file.h"
#include "orange/table/table.h"

File* File::files[MAX_FILE_NUM];
SavedTable* SavedTable::tables[MAX_TBL_NUM];
