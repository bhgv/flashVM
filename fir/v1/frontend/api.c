/* Validate bounded UTF-8/version input and translate scanner/parser errors into caller storage. */
#include "firsyn.h"
#include "firsynp.h"
#include <stdlib.h>
#include <string.h>

extern Error_Func Custom_Error;


extern int firsyn_parser_run(unsigned char *, int);


extern int firsyn_parser_limit_hit(void);
static FirSyntaxDiagnostic *active; /* Borrowed only during one serialized parser invocation. */
/* Generated token/production messages belong to this parser namespace. */
static const char *const messages[] = {
#include "firsyne.h"

};


/* Retain the first syntax error without printing or allocating. */
static void report(int code, int line, int column, long offset)
{
    if (!active || active->message[0])
    {
        return;
    }
    active->code = code;
    active->line = line;
    active->column = column;
    active->offset = offset;
    snprintf(active->message,
             sizeof(active->message),
             "%s",
             code >= 0 && (size_t)code < sizeof(messages) / sizeof(messages[0]) ? messages[code] : "syntax error");
}


/* Reject malformed UTF-8 and embedded NUL before the byte-oriented generated scanner. */
static int valid_utf8(const unsigned char *text, size_t n)
{
    size_t i;
    int remaining = 0;
    unsigned low = 0x80, high = 0xbf;
    for (i = 0; i < n; ++i)
    {
        unsigned c = text[i];
        if (remaining)
        {
            if (c < low || c > high)
            {
                return 0;
            }
            --remaining;
            low = 0x80;
            high = 0xbf;
        }
        else if (!c)
        {
            return 0;
        }
        else if (c < 0x80)
        {
            continue;
        }
        else if (c >= 0xc2 && c <= 0xdf)
        {
            remaining = 1;
        }
        else if (c >= 0xe0 && c <= 0xef)
        {
            remaining = 2;
            low = c == 0xe0 ? 0xa0 : 0x80;
            high = c == 0xed ? 0x9f : 0xbf;
        }
        else if (c >= 0xf0 && c <= 0xf4)
        {
            remaining = 3;
            low = c == 0xf0 ? 0x90 : 0x80;
            high = c == 0xf4 ? 0x8f : 0xbf;
        }
        else
        {
            return 0;
        }
    }
    return !remaining;
}


/* Validate the physical FIR version line, optionally followed by a // comment.
 * data/size borrow already validated UTF-8 bytes. Return one only for an exact
 * version plus LF/CRLF; bare trailing spaces, BOM and preceding lines fail.
 * No allocation, parser state or caller storage is acquired or changed. */
static int valid_header(const unsigned char *data, size_t size)
{
    size_t cursor = 7;
    if (size < 8 || memcmp(data, "FIR 1.0", 7))
    {
        return 0;
    }
    if (data[cursor] == '\n' || (data[cursor] == '\r' && size > 8 && data[cursor + 1] == '\n'))
    {
        return 1;
    }
    while (cursor < size && (data[cursor] == ' ' || data[cursor] == '\t'))
    {
        cursor++;
    }
    if (cursor == 7 || size - cursor < 2 || data[cursor] != '/' || data[cursor + 1] != '/')
    {
        return 0;
    }
    cursor += 2;
    while (cursor < size && data[cursor] != '\r' && data[cursor] != '\n')
    {
        cursor++;
    }
    return cursor < size && (data[cursor] == '\n' ||
           (data[cursor] == '\r' && size - cursor > 1 && data[cursor + 1] == '\n'));
}


/* Parse borrowed bytes synchronously; reset generated state on every invocation. */
int firsyn_check(const void *data, size_t size, FirSyntaxDiagnostic *diagnostic)
{
    FirSyntaxDiagnostic local;
    int ok;
    if (!diagnostic)
    {
        diagnostic = &local;
    }
    memset(diagnostic, 0, sizeof(*diagnostic));
    if ((!data && size) || size > FIR_SYN_INPUT_LIMIT || !valid_utf8((const unsigned char *)data, size))
    {
        diagnostic->code = -1;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "invalid UTF-8/NUL or input exceeds 2 MiB");
        return 0;
    }
    if (!valid_header((const unsigned char *)data, size))
    {
        diagnostic->code = 1401;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "first line must be FIR 1.0");
        return 0;
    }
    active = diagnostic;
    Custom_Error = report;
    ok = firsyn_parser_run((unsigned char *)data, (int)size);
    if (!ok)
    {
        int limit = firsyn_parser_limit_hit();
        diagnostic->code = limit ? limit == 1 ? -3 : -4 : -2;
        snprintf(diagnostic->message,
                 sizeof(diagnostic->message),
                 "%s",
                 limit ? limit == 1 ? "parser nesting limit exceeded" : "parser work limit exceeded"
                       : "scanner allocation failure");
    }
    ok = ok && Successful();
    Custom_Error = 0;
    active = NULL;
    return ok;
}


/* Read from the current stream position; neither rewind nor close caller storage. */
int firsyn_read(FILE *input, FirSyntaxDiagnostic *diagnostic)
{
    unsigned char *data;
    size_t n;
    int ok;
    if (!input)
    {
        return firsyn_check(NULL, 1, diagnostic);
    }
    data = (unsigned char *)malloc(FIR_SYN_INPUT_LIMIT + 1);
    if (!data)
    {
        return firsyn_check(NULL, 1, diagnostic);
    }
    n = fread(data, 1, FIR_SYN_INPUT_LIMIT + 1, input);
    ok = !ferror(input) ? firsyn_check(data, n, diagnostic) : firsyn_check(NULL, 1, diagnostic);
    free(data);
    return ok;
}
