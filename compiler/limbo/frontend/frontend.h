/* Standalone C99 syntax API; no argv, output streams, exit or subprocesses. */
#ifndef limbo_FRONTEND_H
#define limbo_FRONTEND_H
/* UTF-8 scanner units and the 2 MiB input bound require octets and >=32-bit int. */
#include <limits.h>
#if CHAR_BIT != 8 || INT_MAX < 2147483647
#error "This syntax frontend requires 8-bit bytes and at least 32-bit int"
#endif
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct
    {
        int code, line, column;
        long offset;
        char message[256];
    } LimboDiagnostic;
    /* Return 1 accepted / 0 rejected. Optional caller-owned diagnostic.
     * Input bytes/stream remain caller-owned. Sequential calls reset parser state;
     * serialize calls to this parser externally (not reentrant). */
    int limbo_check(const void *bytes, size_t size, LimboDiagnostic *diagnostic);
    int limbo_read(FILE *input, LimboDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
