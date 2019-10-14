
#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif


// #include <rapidjson.h>

#include "rapidjson/rapidjson.h"
#include "fs/fileio/FileManager.h"
#include "fs/fileio/FileTable.h"

#include "fs/utils/MyHashMap.h"
#include "fs/bufmanager/BufPageManager.h"
#include "fs/bufmanager/FindReplace.h"