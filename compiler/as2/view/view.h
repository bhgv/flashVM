#ifndef AS2_VIEW_H
#define AS2_VIEW_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif


/* Copied source position for the first syntax/profile error; no borrowed strings. */
typedef struct
{
    int code;    /* Shared AS2 or display-profile diagnostic code; zero for host bounds. */
    int line;    /* One-based source line when supplied by the parser. */
    int column;  /* One-based source column when supplied by the parser. */
    long offset; /* Byte offset in the source, or zero for host bounds. */
} As2ViewDiagnostic;

/* Compile a <=16 KiB ASCII display-profile unit to caller-owned UTF-8 FIR bytes.
 * Dynamic string values, including Unicode, are bound at execution, not parsed here.
 * Return 1 on success, 0 on syntax/profile/capacity failure. Failure sets *written=0
 * and leaves output untouched. Serialize calls with this library/shared AS2 frontend.
 * No input pointer is retained, no I/O occurs, and no partial output is published. */
int as2_view_compile(const void *bytes,
                     size_t size,
                     unsigned char *output,
                     size_t capacity,
                     size_t *written,
                     As2ViewDiagnostic *diagnostic);
#ifdef __cplusplus
}
#endif
#endif
