/* Exercise the public C99 API against real Coco/R scanner/parser output. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Do not use assert: these checks must execute in Release builds too. */
static void require(int ok, const char *message)
{
    if (!ok)
    {
        fprintf(stderr, "FAIL source diagnostics: %s\n", message);
        exit(1);
    }
}


/* Own private streams per call, verify rollback and return a copied diagnostic. */
static FirSourceDiagnostic
rejected(const char *text, size_t length, FirLanguage language, FirSourceDiagnosticKind kind, size_t capacity)
{
    FILE *input = fir_temporary(), *output = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char error[128], old[9] = "";
    require(input && output, "temporary streams");
    require(fwrite(text, 1, length, input) == length, "write input");
    rewind(input);
    fputs("original", output);
    memset(&diagnostic, 0xff, sizeof(diagnostic));
    require(!fir_source_ex(input, output, language, NULL, capacity ? error : NULL, capacity, &diagnostic),
            "must reject");
    require(diagnostic.kind == kind, "failure kind");
    require(ftell(output) == 8, "rejected translation writes no output");
    rewind(output);
    require(fread(old, 1, 8, output) == 8 && !strcmp(old, "original"), "retain output bytes");
    fclose(input);
    fclose(output);
    return diagnostic;
}


/* Compare exact original-byte locations, independent of scanner tab columns. */
static void span(FirSourceDiagnostic d, size_t offset, size_t length, size_t line, size_t column)
{
    require(d.has_location && d.byte_offset == offset && d.byte_length == length && d.line == line &&
                d.column == column,
            "original UTF-8 byte span");
}


/* Interleave failures and successes to expose stale parser/diagnostic roots. */
int main(void)
{
    int cycle;
    const char unicode[] = "/*\xe4\xb8\xad*/\ttrace(missing);";
    const char duplicate[] = "var a=1;\nvar a=\n2;";
    const char constant[] = "const x=1;\nx =\n2;";
    FirSourceDiagnostic d;
    char *large = (char *)malloc(65537);
    require(large != NULL, "large fixture");
    memset(large, ' ', 65537);
    for (cycle = 0; cycle < 8; ++cycle)
    {
        FILE *input, *output;
        char error[64];
        d = rejected(unicode, strlen(unicode), FIR_AS2, FIR_DIAG_SEMANTIC, cycle % 2 ? 1 : 0);
        span(d, 14, 7, 1, 15);
        require(d.code == 0, "semantic code is zero");
        d = rejected(duplicate, strlen(duplicate), FIR_AS2, FIR_DIAG_SEMANTIC, sizeof(error));
        span(d, 13, 1, 2, 5);
        d = rejected(constant, strlen(constant), FIR_AS3, FIR_DIAG_SEMANTIC, sizeof(error));
        span(d, 11, 1, 2, 1);
        d = rejected("trace(1 +", 9, FIR_AS1, FIR_DIAG_SYNTAX, sizeof(error));
        span(d, 9, 0, 1, 10);
        require(d.code > 0, "Coco/R syntax code retained");
        d = rejected("# bad\r\ntrace(1);", 16, FIR_AS2, FIR_DIAG_SEMANTIC, sizeof(error));
        span(d, 0, 5, 1, 1);
        d = rejected("\t\r\ntrace(missing);", 18, FIR_AS1, FIR_DIAG_SEMANTIC, sizeof(error));
        span(d, 9, 7, 2, 7);
        d = rejected("\xff", 1, FIR_AS1, FIR_DIAG_ENCODING, 0);
        require(!d.has_location && !d.byte_offset && !d.line, "encoding is unlocated");
        d = rejected("trace(1);\0", 10, FIR_AS1, FIR_DIAG_ENCODING, 0);
        require(!d.has_location, "NUL is unlocated");
        d = rejected(large, 65537, FIR_AS1, FIR_DIAG_LIMIT, 0);
        require(!d.has_location, "input limit is unlocated");
        memset(&d, 0xff, sizeof(d));
        require(!fir_source_ex(NULL, NULL, FIR_AS1, NULL, NULL, 0, &d), "invalid arguments");
        require(d.kind == FIR_DIAG_ARGUMENT && !d.has_location && !d.code, "cleared argument result");
        input = fir_temporary();
        output = fir_temporary();
        require(input && output, "success streams");
        fputs("trace(42);", input);
        rewind(input);
        require(fir_source_ex(input, output, FIR_AS2, NULL, error, sizeof(error), &d), "success after errors");
        require(d.kind == FIR_DIAG_NONE && !d.has_location && !d.byte_length && !error[0], "success clears diagnostic");
        rewind(input);
        require(fir_source(input, output, FIR_AS2, NULL, error, sizeof(error)), "legacy wrapper retained");
        fclose(input);
        fclose(output);
    }
    free(large);
    puts("PASS source diagnostic API: 8 repeated failure/success cycles");
    return 0;
}
