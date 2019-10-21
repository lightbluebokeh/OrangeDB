#pragma once

#include <defs.h>

namespace Index {

    rid_t get_first_unused(String filename);

    bool insert(rid_t rid);

    bool insert(int fileid, long long offset);

    bool remove(rid_t rid);

    bool remove(int fileid, long long offset);

}  // namespace Index