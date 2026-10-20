/* Adapt legacy scanner fatal paths into recoverable library failures.
 * No upstream scanner/runtime is vendored or modified. */
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "limbop.h"
static jmp_buf scanner_failure;
static void scanner_abort(int status) { (void)status; longjmp(scanner_failure, 1); }
static int scanner_message(FILE *stream, const char *format, ...)
{ (void)stream; (void)format; return 0; }
#define exit scanner_abort
#define fprintf scanner_message
#include "limbos.c"
#undef exit
#undef fprintf
extern void limbo_parser_reset(void);
int limbo_parser_run(unsigned char *text, int size)
{
    if (setjmp(scanner_failure)) return 0;
    limbo_parser_reset(); Parse(text, size); return 1;
}
