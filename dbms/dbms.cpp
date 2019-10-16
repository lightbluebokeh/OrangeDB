#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif

#include "defs.h"
#include <record/file.h>

API void f() {}