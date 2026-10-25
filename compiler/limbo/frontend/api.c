#include "frontend.h"
#include "limbop.h"
#include <stdlib.h>
#include <string.h>


extern Error_Func Custom_Error; /* Generated parser callback slot; cleared after each check. */


extern int limbo_parser_run(unsigned char *, int); /* Run the reset scanner/parser on borrowed bytes. */
extern int limbo_parser_limit_hit(void);           /* Report nesting/work exhaustion after a failed run. */
static LimboDiagnostic *active;                    /* Borrowed diagnostic for the current serialized check. */
static const char *const messages[] = { /* Generated diagnostic text indexed by parser error code. */
#include "limboe.h"

};


/* Record only the first generated scanner/parser error in the active diagnostic.
 * Parameters: code selects the message; line, column and offset identify its
 * source location as reported by Coco/R. No input buffer is retained.
 * Returns: nothing; an absent/already populated diagnostic is left unchanged. */
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
    if (code == 1302)
    {
        snprintf(active->message, sizeof(active->message), "variadic parameter must be last");
        return;
    }
    if (code == 1301)
    {
        snprintf(active->message, sizeof(active->message), "array without initializer requires a size");
        return;
    }
    snprintf(active->message,
             sizeof(active->message),
             "%s",
             code >= 0 && (size_t)code < sizeof(messages) / sizeof(messages[0]) ? messages[code] : "syntax error");
}


/* Validate the complete source before handing bytes to the generated scanner.
 * Parameters: text borrows n bytes; a null pointer is permitted only when n is zero.
 * Returns: one for complete scalar UTF-8 without NUL, zero for malformed input.
 * This helper does not allocate or alter the supplied bytes. */
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


/* Check LIMBO syntax with the reset, non-reentrant Coco/R frontend.
 * Parameters: data borrows size source bytes; diagnostic is an optional caller-owned
 * output and is reset before validation. A null data pointer requires size zero.
 * Returns: one when accepted, zero for invalid input, syntax, scanner allocation
 * failure or a parser limit. Serialize calls to this frontend externally.
 * The diagnostic callback borrows its output only for the duration of this call. */
int limbo_check(const void *data, size_t size, LimboDiagnostic *diagnostic)
{
    LimboDiagnostic local;
    int ok;
    if (!diagnostic)
    {
        diagnostic = &local;
    }
    memset(diagnostic, 0, sizeof(*diagnostic));
    if ((!data && size) || size > 2u * 1024u * 1024u || !valid_utf8((const unsigned char *)data, size))
    {
        diagnostic->code = -1;
        snprintf(diagnostic->message, sizeof(diagnostic->message), "invalid UTF-8/NUL or input exceeds 2 MiB");
        return 0;
    }
    active = diagnostic;
    Custom_Error = report;
    ok = limbo_parser_run((unsigned char *)data, (int)size);
    if (!ok)
    {
        int limit = limbo_parser_limit_hit();
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


/* Read a bounded stream and pass its contents to limbo_check.
 * Parameters: input is a borrowed FILE left open; diagnostic is optional caller-owned
 * output. The stream advances through at most the source limit plus one byte.
 * Returns: one for accepted syntax, zero for read/allocation/input/parser failure.
 * The temporary source buffer is freed on both accepted and rejected parses. */
int limbo_read(FILE *input, LimboDiagnostic *diagnostic)
{
    unsigned char *data;
    size_t n;
    int ok;
    if (!input)
    {
        return limbo_check(NULL, 1, diagnostic);
    }
    data = (unsigned char *)malloc(2u * 1024u * 1024u + 1);
    if (!data)
    {
        return limbo_check(NULL, 1, diagnostic);
    }
    n = fread(data, 1, 2u * 1024u * 1024u + 1, input);
    ok = !ferror(input) ? limbo_check(data, n, diagnostic) : limbo_check(NULL, 1, diagnostic);
    free(data);
    return ok;
}
