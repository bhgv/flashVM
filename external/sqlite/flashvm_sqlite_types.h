#ifndef FVM_SQLITE_TYPES_H
#define FVM_SQLITE_TYPES_H
/* Keep the same 64-bit SQLite ABI in the C library and C++98 adapter.
 * LP64 uses ordinary C89 long; 32-bit targets require a native system type. */
#include <limits.h>
#ifndef SQLITE_INT64_TYPE
#if ULONG_MAX > 0xffffffffUL
#define SQLITE_INT64_TYPE long
#define SQLITE_UINT64_TYPE unsigned long
#elif defined(_MSC_VER)
#define SQLITE_INT64_TYPE __int64
#define SQLITE_UINT64_TYPE unsigned __int64
#else
#include <stdint.h>
#define SQLITE_INT64_TYPE int64_t
#define SQLITE_UINT64_TYPE uint64_t
#endif
#endif
#endif
