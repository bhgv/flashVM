/* CLI translates paths/options to the same public syntax API used by an IDE. */
#include "firsyn.h"
#include <string.h>
int main(int argc, char **argv)
{
    FILE *input;
    FirSyntaxDiagnostic diagnostic;
    int ok;
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: firsyn_parse INPUT.fir\nStructural FIR 1.0 syntax only; no IR validation, "
             "optimization or object output.\nC API: "
             "firsyn_check(bytes,size,diagnostic), firsyn_read(stream,diagnostic).\nLimit: 2 MiB, "
             "UTF-8 "
             "without NUL; serialize parser calls.\nExit: 0 accepted, 1 rejected, 2 usage/I/O "
             "error.\nSee fir/v1/frontend/README.md and README.ru.md.");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("firsyn_parse 0.1.1 (C99 syntax API)");
        return 0;
    }
    if (argc != 2)
        return 2;
    input = fopen(argv[1], "rb");
    if (!input)
    {
        perror(argv[1]);
        return 2;
    }
    ok = firsyn_read(input, &diagnostic);
    if (fclose(input))
        return 2;
    if (!ok)
        fprintf(stderr, "%s:%d:%d: %s E%d (byte %ld)\n", argv[1], diagnostic.line,
                diagnostic.column, diagnostic.message, diagnostic.code, diagnostic.offset);
    return ok ? 0 : 1;
}
