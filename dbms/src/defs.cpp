#include <defs.h>
#include <fs/file/file.h>

File *File::file[MAX_FILE_NUM];

void ensure(bool cond, const String& msg) {
    if (cond == 0) {
        std::cerr << RED << "failed: " << RESET << msg << std::endl;
        throw msg;                            
    }
}