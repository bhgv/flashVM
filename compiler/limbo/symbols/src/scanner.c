/* Intercept legacy scanner allocation aborts without vendoring its runtime. */
#include "sblmapp.h"
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
static jmp_buf scanner_failure;
/* Return scanner fatal paths to the owning C API call. */
static void scanner_abort(int status)
{
    (void)status;
    longjmp(scanner_failure, 1);
}
/* Suppress scanner console diagnostics; the API reports failure to its caller. */
static int scanner_message(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}
#define exit scanner_abort
#define fprintf scanner_message
#include "sblmaps.c"
#undef exit
#undef fprintf
extern void sblmap_parser_reset(void);
/* The caller serializes this process-local generated parser. */
int sblmap_parser_run(unsigned char *text, int size)
{
    if (setjmp(scanner_failure))
        return 0;
    sblmap_parser_reset();
    Parse(text, size);
    return 1;
}
