/* In-process source compiler: staged output and first-error diagnostics.
 * All parser calls are serialized by the host; this module owns no caller stream. */
#include "fir.h"
#include "src1p.h"
#include "src1s.h"
#include "lower.h"
#include <stdlib.h>
#include <string.h>

extern int fir_utf8(const unsigned char *, size_t);
extern int source_parser_run(unsigned char *, int);
extern Error_Func Custom_Error;

/* Borrowed only during fir_source_ex; retired on every return path. */
static char *active_error;
static size_t active_capacity;
static FirSourceDiagnostic *active_diagnostic;
static const unsigned char *source_bytes;
static size_t source_length;
static int error_recorded;

/* Preserve the first error independently of the caller's optional text buffer.
 * Derive canonical byte columns from original bytes, never scanner tab expansion. */
void source_error(FirSourceDiagnosticKind kind, int code, long offset, size_t length,
                  const char *message)
{
    size_t i, line = 1, column = 1;
    int located;
    if (error_recorded)
        return;
    error_recorded = 1;
    if (active_error && active_capacity)
        snprintf(active_error, active_capacity, "%s", message);
    if (!active_diagnostic)
        return;
    active_diagnostic->kind = kind;
    active_diagnostic->code = code;
    located = source_bytes && offset >= 0 && (size_t)offset <= source_length &&
              length <= source_length - (size_t)offset;
    if (!located)
        return;
    for (i = 0; i < (size_t)offset; ++i)
    {
        if (source_bytes[i] == '\n')
        {
            ++line;
            column = 1;
        }
        else
            ++column;
    }
    active_diagnostic->has_location = 1;
    active_diagnostic->byte_offset = (size_t)offset;
    active_diagnostic->byte_length = length;
    active_diagnostic->line = line;
    active_diagnostic->column = column;
}

/* Adapt Coco/R's callback while retaining its syntax code and legacy message. */
static void report(int code, int line, int column, long offset)
{
    char message[192];
    size_t length = S_NextLen > 0 ? (size_t)S_NextLen : 0;
    if (offset >= 0 && (size_t)offset == source_length)
        length = 0;
    snprintf(message, sizeof(message), "source:%d:%d: syntax E%d (byte %ld)",
             line, column, code, offset);
    source_error(FIR_DIAG_SYNTAX, code, offset, length, message);
}

/* Validate a literal portable module name without changing input/output streams. */
static int valid_name(const char *name)
{
    size_t i, length = strlen(name);
    if (!length || length > 254)
        return 0;
    for (i = 0; i < length; ++i)
    {
        unsigned ch = (unsigned char)name[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_' ||
              (i && ((ch >= '0' && ch <= '9') || ch == '.'))))
            return 0;
    }
    return 1;
}

/* Compile one bounded snapshot into a private stream before copying it out.
 * Rejected input/translation preserves output; final output I/O failure may leave
 * a prefix. The caller owns both streams and serializes all generated parsers. */
int fir_source_ex(FILE *input, FILE *output, FirLanguage language, const char *name,
                  char *error, size_t capacity, FirSourceDiagnostic *diagnostic)
{
    unsigned char *text = NULL;
    FILE *temporary = NULL;
    size_t length;
    int ok = 0;
    active_error = error;
    active_capacity = capacity;
    active_diagnostic = diagnostic;
    source_bytes = NULL;
    source_length = 0;
    error_recorded = 0;
    if (error && capacity)
        error[0] = 0;
    if (diagnostic)
        memset(diagnostic, 0, sizeof(*diagnostic));
    if (!name)
        name = "tutorial";
    if (!input || !output || language < FIR_AS1 || language > FIR_LIMBO || !valid_name(name))
    {
        source_error(FIR_DIAG_ARGUMENT, 0, -1, 0, "invalid source arguments");
        goto done;
    }
    text = (unsigned char *)calloc(65538, 1);
    temporary = fir_temporary();
    if (!text || !temporary)
    {
        source_error(FIR_DIAG_RESOURCE, 0, -1, 0, "temporary allocation failure");
        goto done;
    }
    length = fread(text, 1, 65537, input);
    if (ferror(input))
    {
        source_error(FIR_DIAG_INPUT, 0, -1, 0, "source read error");
        goto done;
    }
    if (length > 65536)
    {
        source_error(FIR_DIAG_LIMIT, 0, -1, 0, "source exceeds 64 KiB");
        goto done;
    }
    if (!fir_utf8(text, length))
    {
        source_error(FIR_DIAG_ENCODING, 0, -1, 0, "invalid UTF-8 or embedded NUL");
        goto done;
    }
    source_bytes = text;
    source_length = length;
    source_config(temporary, (int)language);
    Custom_Error = report;
    fprintf(temporary,
            "FIR 1.0\nmodule @%s\nsource_language %s\nsource_semantics %s\nfunc @main() -> void "
            "{\n^entry:\n", name,
            language == FIR_AS1 ? "as1" : language == FIR_AS2 ? "as2" :
            language == FIR_AS3 ? "as3" : "limbo",
            language == FIR_LIMBO ? "limbo_i32" : "avm1_numeric");
    if (!source_parser_run(text, (int)length))
    {
        source_error(FIR_DIAG_RESOURCE, 0, -1, 0, "scanner allocation failure");
        goto done;
    }
    fputs("    return\n}\n", temporary);
    if (!Successful() || source_failed() || error_recorded)
        goto done;
    if (ferror(temporary) || fflush(temporary) || fseek(temporary, 0, SEEK_SET))
    {
        source_error(FIR_DIAG_OUTPUT, 0, -1, 0, "temporary FIR write/seek failure");
        goto done;
    }
    while ((length = fread(text, 1, 65536, temporary)) != 0)
    {
        if (fwrite(text, 1, length, output) != length)
        {
            source_error(FIR_DIAG_OUTPUT, 0, -1, 0, "FIR output write failure");
            goto done;
        }
    }
    if (ferror(temporary) || ferror(output))
    {
        source_error(FIR_DIAG_OUTPUT, 0, -1, 0, "FIR stream I/O failure");
        goto done;
    }
    ok = 1;
done:
    Custom_Error = 0;
    source_release();
    active_error = NULL;
    active_capacity = 0;
    active_diagnostic = NULL;
    source_bytes = NULL;
    source_length = 0;
    if (temporary)
        fclose(temporary);
    free(text);
    return ok;
}

/* Preserve source compatibility for hosts that only consume legacy text errors. */
int fir_source(FILE *input, FILE *output, FirLanguage language, const char *name,
               char *error, size_t capacity)
{
    return fir_source_ex(input, output, language, name, error, capacity, NULL);
}
