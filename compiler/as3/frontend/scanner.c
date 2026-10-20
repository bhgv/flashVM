/* Adapt legacy scanner fatal paths into recoverable library failures.
 * No upstream scanner/runtime is vendored or modified. */
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "as3p.h"
static jmp_buf scanner_failure;
static unsigned nesting;
static unsigned long work;
static int limit_hit;
/* Separate from generated scanner errors; no allocation or printing here. */
void as3_syntax_enter(void)
{
    if (++nesting > 512)
    {
        limit_hit = 1;
        longjmp(scanner_failure, 1);
    }
    if (++work > 4000000UL)
    {
        limit_hit = 2;
        longjmp(scanner_failure, 1);
    }
}
void as3_syntax_leave(void)
{
    --nesting;
}
int as3_parser_limit_hit(void)
{
    return limit_hit;
}
static void scanner_abort(int status)
{
    (void)status;
    longjmp(scanner_failure, 1);
}
static int scanner_message(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return 0;
}
#define exit scanner_abort
#define fprintf scanner_message
#include "as3s.c"
#undef exit
#undef fprintf
extern void as3_parser_reset(void);
int as3_parser_run(unsigned char *text, int size)
{
    nesting = 0;
    work = 0;
    limit_hit = 0;
    if (setjmp(scanner_failure))
        return 0;
    as3_parser_reset();
    Parse(text, size);
    return 1;
}

/* Split an already scanned >-prefixed token without modifying the source buffer.
 * All bytes are ASCII on one line; restore the cursor just after the first >. */
void as3_split_greater(void)
{
    if (S_NextLen > 1 && S_CurrentCh(S_NextPos) == '>')
    {
        S_CurrCol -= S_NextLen - 1;
        S_BuffPos = S_NextPos + 1;
        S_ch = S_CurrentCh(S_BuffPos);
        S_NextLen = 1;
    }
}

/* Parser-selected lexical goal: / and /= may begin a regexp only in operand position.
 * Escaped delimiters and character classes are opaque to the normal generated scanner. */
int as3_scan_regexp(void)
{
    long pos = S_NextPos + 1, end;
    unsigned int flags = 0;
    int in_class = 0, ch;
    if (S_CurrentCh(S_NextPos) != '/')
        return 0;
    for (;; ++pos)
    {
        ch = S_CurrentCh(pos);
        if (!ch || ch == 10 || ch == 13 ||
            (ch == 226 && S_CurrentCh(pos + 1) == 128 &&
             (S_CurrentCh(pos + 2) == 168 || S_CurrentCh(pos + 2) == 169)))
            return 0;
        if (ch == 92)
        {
            ch = S_CurrentCh(++pos);
            if (!ch || ch == 10 || ch == 13)
                return 0;
        }
        else if (ch == '[')
            in_class = 1;
        else if (ch == ']')
            in_class = 0;
        else if (ch == '/' && !in_class)
            break;
    }
    end = pos + 1;
    for (;; ++end)
    {
        unsigned int bit;
        ch = S_CurrentCh(end);
        if (ch == 'g')
            bit = 1;
        else if (ch == 'i')
            bit = 2;
        else if (ch == 'm')
            bit = 4;
        else if (ch == 's')
            bit = 8;
        else if (ch == 'x')
            bit = 16;
        else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
                 ch == '_' || ch == '$' || ch >= 128)
            return 0;
        else
            break;
        if (flags & bit)
            return 0;
        flags |= bit;
    }
    while (S_BuffPos < end)
    {
        S_NextCh();
    }
    S_NextLen = (int)(end - S_NextPos);
    return 1;
}

/* Advance XML raw text to a parser-selected source position. Interpolated AS has
 * already advanced the same scanner; preserve byte/line diagnostics without a second buffer. */
void as3_seek_source(long position)
{
    while (S_BuffPos < position)
    {
        S_NextCh();
    }
    S_NextPos = position;
    S_NextLen = 0;
    S_NextLine = S_CurrLine;
    S_NextCol = S_CurrCol - 1;
}
