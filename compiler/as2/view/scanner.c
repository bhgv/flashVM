/* Recover generated scanner allocation failures at the library boundary. */
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "viewp.h"

static jmp_buf scanner_failure; /* One active serialized compile call, never a thread-local promise. */

/* Generated scanner fatal paths become a failed library call, without terminating the host. */
static void scanner_abort(int status)
{
    (void)status;
    longjmp(scanner_failure, 1);
}

/* Suppress generated fatal output; the library reports failure to its caller. */
static int scanner_message(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}

#define exit scanner_abort
#define fprintf scanner_message
#include "views.c"
#undef exit
#undef fprintf

/* Parse borrowed bytes under an allocation-failure recovery frame. Return one or zero. */
int view_parser_run(unsigned char *source, int length)
{
    if (setjmp(scanner_failure))
    {
        return 0;
    }
    Parse(source, length);
    return 1;
}
