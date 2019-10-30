#include <defs.h>
#include <exception>
#include <fs/file/file.h>
#include <orange/table/table.h>

File* File::files[MAX_FILE_NUM];
Table* Table::tables[MAX_TBL_NUM];

void ensure(bool cond, const String& msg) {
    if (cond == 0) {
        std::cerr << RED << "failed: " << RESET << msg << std::endl;
        throw std::exception(msg.c_str());
    }
}
