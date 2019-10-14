#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif

#include "record/File.h"
#include "defs.h"

API void f() {}