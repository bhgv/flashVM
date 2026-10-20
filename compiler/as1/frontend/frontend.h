/* Standalone C99 syntax API; no argv, output streams, exit or subprocesses. */
#ifndef as1_FRONTEND_H
#define as1_FRONTEND_H
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { int code, line, column; long offset; char message[256]; } As1Diagnostic;
/* Return 1 accepted / 0 rejected. Optional caller-owned diagnostic.
 * Input bytes/stream remain caller-owned. Sequential calls reset parser state;
 * serialize calls to this parser externally (not reentrant). */
int as1_check(const void *bytes, size_t size, As1Diagnostic *diagnostic);
int as1_read(FILE *input, As1Diagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
