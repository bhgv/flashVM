/* Adapt legacy scanner fatal paths into recoverable library failures.
 * No upstream scanner/runtime is vendored or modified. */
#include "firsyn.h"
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "firsynp.h"
static jmp_buf scanner_failure;
static unsigned nesting;
static unsigned long work;
static int limit_hit;
/* Separate from generated scanner errors; no allocation or printing here. */
void firsyn_syntax_enter(void)
{
    if (++nesting > FIR_SYN_NESTING_LIMIT)
    {
        limit_hit = 1;
        longjmp(scanner_failure, 1);
    }
    if (++work > FIR_SYN_WORK_LIMIT)
    {
        limit_hit = 2;
        longjmp(scanner_failure, 1);
    }
}
/* Balance successful entries; longjmp failures reset counters on the next invocation. */
void firsyn_syntax_leave(void)
{
    --nesting;
}
/* Report the reason retained across the scanner longjmp boundary. */
int firsyn_parser_limit_hit(void)
{
    return limit_hit;
}
/* Convert upstream fatal allocation paths into a returned library failure. */
static void scanner_abort(int status)
{
    (void)status;
    longjmp(scanner_failure, 1);
}
/* Suppress upstream fatal printing; the public API owns diagnostics. */
static int scanner_message(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}
#define exit scanner_abort
#define fprintf scanner_message
#include "firsyns.c"
#undef exit
#undef fprintf
extern void firsyn_parser_reset(void);
/* One recovery boundary protects parser nesting/work and scanner allocation failure. */
int firsyn_parser_run(unsigned char *text, int size)
{
    nesting = 0;
    work = 0;
    limit_hit = 0;
    if (setjmp(scanner_failure))
        return 0;
    firsyn_parser_reset();
    Parse(text, size);
    return 1;
}
