/* Adapt legacy scanner fatal paths into recoverable library failures.
 * No upstream scanner/runtime is vendored or modified. */
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "as2p.h"

static jmp_buf scanner_failure;
static unsigned nesting;
static unsigned long work;
static int limit_hit;


/* Separate from generated scanner errors; no allocation or printing here. */
void as2_syntax_enter(void)
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


void as2_syntax_leave(void)
{
    --nesting;
}


int as2_parser_limit_hit(void)
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
#include "as2s.c"

#undef exit
#undef fprintf


extern int as2_string_valid(long, int);

/* Source locations count CRLF once, CR/LF/LS/PS as line breaks, and UTF-8 byte
 * columns with eight-column tabs. The monotonic cursor visits each source byte
 * once, including bytes bypassed by regexp/XML scanner goals. */
static long lexical_position;
static int lexical_line, lexical_column;


static void lexical_location(long target)
{
    while (lexical_position < target)
    {
        int ch = S_CurrentCh(lexical_position++);
        if (ch == 13)
        {
            ++lexical_line;
            lexical_column = 1;
        }
        else if (ch == 10)
        {
            if (lexical_position < 2 || S_CurrentCh(lexical_position - 2) != 13)
            {
                ++lexical_line;
            }
            lexical_column = 1;
        }
        else if (ch == 226 && S_CurrentCh(lexical_position) == 128 &&
                 (S_CurrentCh(lexical_position + 1) == 168 || S_CurrentCh(lexical_position + 1) == 169) &&
                 lexical_position + 2 <= target)
        {
            lexical_position += 2;
            ++lexical_line;
            lexical_column = 1;
        }
        else if (ch == 9)
        {
            lexical_column += 8 - ((lexical_column - 1) % 8);
        }
        else
        {
            ++lexical_column;
        }
    }
    S_NextLine = lexical_line;
    S_NextCol = lexical_column;
}


/* Publish a token only after its canonical source location is available. */
static int lexical_token(int token)
{
    lexical_location(S_NextPos);
    if (token == textSym && !as2_string_valid(S_NextPos, S_NextLen))
    {
        SynError(1107);
    }
    return token;
}


extern void as2_parser_reset(void);


int as2_parser_run(unsigned char *text, int size)
{
    nesting = 0;
    work = 0;
    limit_hit = 0;
    lexical_position = 0;
    lexical_line = 1;
    lexical_column = 1;
    if (setjmp(scanner_failure))
    {
        return 0;
    }
    as2_parser_reset();
    Parse(text, size);
    return 1;
}


extern int as2_identifier_valid(long, int);


extern int as2_identifier_space(long, int *);


/* Unicode whitespace can split a byte-level identifier. Rewind only within its
 * already scanned line, retaining original byte offsets and token ownership. */
static void identifier_cursor(long position)
{
    S_BuffPos = position;
    S_ch = S_CurrentCh(position);
    S_CurrLine = S_NextLine;
    S_CurrCol = S_NextCol + 1 + (int)(position - S_NextPos);
    if (S_ch == 10)
    {
        ++S_CurrLine;
        S_CurrCol = 1;
        S_LineStart = position + 1;
    }
    else if (S_LineStart > position)
    {
        long start = position;
        while (start && S_CurrentCh(start - 1) != 10)
        {
            --start;
        }
        S_LineStart = start;
    }
}


/* Keep Coco/R's DFA and keyword table authoritative; add scalar checks and
 * whitespace boundaries which a byte-character set cannot express. */
int as2_lex_get(void)
{
    long previous_pos = S_NextPos;
    int previous_len = S_NextLen, previous_line = S_NextLine, previous_col = S_NextCol;
    for (;;)
    {
        int token = S_Get();
        long pos, end;
        if (token == linecommentSym)
        {
            long offset, limit = S_NextPos + S_NextLen;
            for (offset = S_NextPos; offset < limit; ++offset)
            {
                int line;
                if (as2_identifier_space(offset, &line) && line)
                {
                    identifier_cursor(offset);
                    S_NextLen = (int)(offset - S_NextPos);
                    break;
                }
            }
            return lexical_token(token);
        }
        if (token != identSym)
        {
            int line, width = as2_identifier_space(S_NextPos, &line);
            if (!width)
            {
                return lexical_token(token);
            }
            identifier_cursor(S_NextPos + width);
            if (line)
            {
                ++S_CurrLine;
                S_CurrCol = 1;
                S_LineStart = S_BuffPos;
            }
        }
        else
        {
            pos = S_NextPos;
            end = pos + S_NextLen;
            while (pos < end)
            {
                int line, width = as2_identifier_space(pos, &line);
                if (width)
                {
                    if (pos != S_NextPos)
                    {
                        identifier_cursor(pos);
                        S_NextLen = (int)(pos - S_NextPos);
                        break;
                    }
                    identifier_cursor(pos + width);
                    if (line)
                    {
                        ++S_CurrLine;
                        S_CurrCol = 1;
                        S_LineStart = S_BuffPos;
                    }
                    break;
                }
                /* Only inspect potential UTF-8 lead bytes or ASCII separators.
                 * Continuation bytes cannot begin a whitespace scalar. */
                ++pos;
            }
            if (pos != S_NextPos)
            {
                lexical_location(S_NextPos);
                if (!as2_identifier_valid(S_NextPos, S_NextLen))
                {
                    SynError(1105);
                }
                return lexical_token(CheckLiteral(identSym));
            }
        }
        /* A discarded whitespace token must not replace the previous token.
         * This preserves ASI's exact source interval, including Unicode LS/PS. */
        S_NextPos = previous_pos;
        S_NextLen = previous_len;
        S_NextLine = previous_line;
        S_NextCol = previous_col;
    }
}
