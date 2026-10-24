/* CLI for Coco/R numeric source profiles. JSON diagnostics are opt-in, while
 * the C99 source API and legacy human-readable invocation remain available. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>

/* Print a bounded UTF-8 message as JSON, escaping control bytes and quotes. */
static void json_string(FILE *stream, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    fputc('"', stream);
    for (; *p; ++p)
    {
        if (*p == '"' || *p == '\\')
            fprintf(stream, "\\%c", *p);
        else if (*p < 32)
            fprintf(stream, "\\u%04x", (unsigned)*p);
        else
            fputc(*p, stream);
    }
    fputc('"', stream);
}

/* Emit exactly one record on a parsed request's failure, never a success log. */
static void report(int json, const FirSourceDiagnostic *diagnostic, const char *message)
{
    static const char *kinds[] = {"none", "argument", "input", "encoding", "limit",
                                "resource", "syntax", "semantic", "output"};
    if (!json)
    {
        fprintf(stderr, "%s\n", message);
        return;
    }
    fprintf(stderr, "{\"schema\":1,\"kind\":\"%s\",\"code\":%d,\"has_location\":%s,"
            "\"byte_offset\":%zu,\"byte_length\":%zu,\"line\":%zu,\"column\":%zu,\"message\":",
            kinds[diagnostic->kind], diagnostic->code,
            diagnostic->has_location ? "true" : "false", diagnostic->byte_offset,
            diagnostic->byte_length, diagnostic->line, diagnostic->column);
    json_string(stderr, message);
    fputs("}\n", stderr);
}

/* Validate options, stage compilation, and open the destination only on success.
 * Output copy failures can leave a prefix; source rejection preserves old files. */
int main(int argc, char **argv)
{
    FILE *input = NULL, *temporary = NULL, *output = NULL;
    FILE *debug = NULL, *debug_file = NULL;
    const char *debug_path = NULL;
    unsigned char buffer[8192];
    size_t length;
    int json = 0, have_module = 0, i, status = 2;
    FirLanguage language;
    const char *module = NULL;
    char error[512] = "source operation failed";
    FirSourceDiagnostic diagnostic;
    memset(&diagnostic, 0, sizeof(diagnostic));
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("fir1_source 0.5.5 (numeric tutorial profiles; diagnostic schema 1)");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: fir1_source as1|as2|as3|limbo --input SOURCE OUTPUT.fir\n"
             "                   [--module NAME] [--diagnostics-json] [-g DEBUG.json]\n"
             "Numeric profiles: initialized names, unary +/-, + - *, groups; AS / and /=.\n"
             "AS3 const; Limbo lexical blocks and value(): int tutorial ABI.\n"
             "Not full AS/Limbo languages; AS3 emits AVM1 compatibility, not ABC.\n"
             "Limits: 64 KiB, 255-byte tokens, 256 names, 1000 SSA values, depth 64.\n"
             "JSON: one schema-1 stderr record on failure; original UTF-8 byte spans.\n"
             "Options may appear in either order; duplicates are rejected.\n"
             "Exit: 0 success, 1 translation/output failure, 2 CLI/setup failure.\n"
             "See doc/SOURCE_DIAGNOSTICS.md and doc/SOURCE_SYNTAX.md.");
        return 0;
    }
    if (argc < 5 || strcmp(argv[2], "--input"))
        goto usage;
    if (!strcmp(argv[1], "limbo"))
        language = FIR_LIMBO;
    else if (!strcmp(argv[1], "as1") || !strcmp(argv[1], "as2") || !strcmp(argv[1], "as3"))
        language = (FirLanguage)(argv[1][2] - '0');
    else
        goto usage;
    for (i = 5; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--diagnostics-json") && !json)
            json = 1;
        else if (!strcmp(argv[i], "--module") && !have_module && i + 1 < argc)
        {
            module = argv[++i];
            have_module = 1;
        }
        else if (!strcmp(argv[i], "-g") && !debug_path && i + 1 < argc)
            debug_path = argv[++i];
        else
            goto usage;
    }
    if (!strcmp(argv[3], argv[4]) || (debug_path &&
        (!strcmp(debug_path, argv[3]) || !strcmp(debug_path, argv[4]))))
    {
        diagnostic.kind = FIR_DIAG_ARGUMENT;
        strcpy(error, "different input/output paths required");
        goto done;
    }
    input = fopen(argv[3], "rb");
    if (!input)
    {
        diagnostic.kind = FIR_DIAG_INPUT;
        strcpy(error, "cannot open source input");
        goto done;
    }
    temporary = fir_temporary();
    if (debug_path)
        debug = fir_temporary();
    if (!temporary || (debug_path && !debug))
    {
        diagnostic.kind = FIR_DIAG_RESOURCE;
        strcpy(error, "cannot create temporary FIR output");
        goto done;
    }
    status = 1;
    if (!fir_source_debug(input, temporary, language, module, error, sizeof(error), &diagnostic, debug))
        goto done;
    if (fflush(temporary) || fseek(temporary, 0, SEEK_SET))
        goto output_error;
    output = fopen(argv[4], "wb");
    if (!output)
        goto output_error;
    while ((length = fread(buffer, 1, sizeof(buffer), temporary)) != 0)
    {
        if (fwrite(buffer, 1, length, output) != length)
            goto output_error;
    }
    if (ferror(temporary))
        goto output_error;
    if (fclose(output))
    {
        output = NULL;
        goto output_error;
    }
    output = NULL;
    if (debug)
    {
        if (fflush(debug) || fseek(debug, 0, SEEK_SET))
            goto output_error;
        debug_file = fopen(debug_path, "wb");
        if (!debug_file)
            goto output_error;
        while ((length = fread(buffer, 1, sizeof(buffer), debug)) != 0)
        {
            if (fwrite(buffer, 1, length, debug_file) != length)
                goto output_error;
        }
        if (ferror(debug))
            goto output_error;
        if (fclose(debug_file))
        {
            debug_file = NULL;
            goto output_error;
        }
        debug_file = NULL;
    }
    status = 0;
    goto done;
output_error:
    memset(&diagnostic, 0, sizeof(diagnostic));
    diagnostic.kind = FIR_DIAG_OUTPUT;
    strcpy(error, "FIR output I/O failure");
done:
    if (input)
        fclose(input);
    if (temporary)
        fclose(temporary);
    if (output)
        fclose(output);
    if (debug)
        fclose(debug);
    if (debug_file)
        fclose(debug_file);
    if (status)
        report(json, &diagnostic, error[0] ? error : "source operation failed");
    return status;
usage:
    fputs("usage: fir1_source as1|as2|as3|limbo --input SOURCE OUTPUT.fir "
          "[--module NAME] [--diagnostics-json] [-g DEBUG.json]\n", stderr);
    return 2;
}
