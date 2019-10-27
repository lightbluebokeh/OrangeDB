#ifdef _WIN32
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C"
#endif

#include <orange/table/table.h>

API void f() {}
