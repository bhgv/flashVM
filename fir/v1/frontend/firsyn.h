/* Standalone C99 syntax API; no argv, output streams, exit or subprocesses. */
#ifndef FIR_SYN_H
#define FIR_SYN_H
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
#define FIR_SYN_INPUT_LIMIT (2u * 1024u * 1024u) /* Maximum borrowed UTF-8 source bytes. */
#define FIR_SYN_NESTING_LIMIT 512U               /* Maximum active guarded grammar calls. */
#define FIR_SYN_WORK_LIMIT 4000000UL             /* Maximum guarded production visits per call. */
#define FIR_SYN_MESSAGE_SIZE 256U /* Diagnostic storage including the trailing NUL. */
    /* Caller-owned first diagnostic; byte coordinates are not Unicode character columns. */
    typedef struct
    {
        int code; /* Generated syntax error, 1401 version error, or negative resource/input code. */
        int line; /* One-based line where available; zero for resource/input validation failure. */
        int column;  /* Scanner byte column, not display width or a Unicode code-point index. */
        long offset; /* Original source byte offset where available. */
        char message[FIR_SYN_MESSAGE_SIZE]; /* Always NUL-terminated diagnostic text. */
    } FirSyntaxDiagnostic;
    /* Return 1 accepted / 0 rejected. Optional caller-owned diagnostic.
     * Input bytes/stream remain caller-owned. Sequential calls reset parser state;
     * serialize calls to this parser externally (not reentrant). */
    int firsyn_check(const void *bytes, size_t size, FirSyntaxDiagnostic *diagnostic);
    int firsyn_read(FILE *input, FirSyntaxDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
