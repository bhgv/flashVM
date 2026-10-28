/* CLI translates paths/options to the same public syntax API used by an IDE. */
#include "frontend.h"
#include <string.h>


int main(int argc, char **argv)
{
    FILE *input;
    LimboDiagnostic diagnostic;
    int ok;
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: limbo_parse SOURCE\nSyntax only, no FIR/bytecode/SWF output.\nC API: "
             "limbo_check(bytes,size,diagnostic), limbo_read(stream,diagnostic).\nLimit: 2 MiB, "
             "UTF-8 without NUL; serialize parser calls.\nExit: 0 accepted, 1 rejected, 2 "
             "usage/I/O error.\nSee compiler/limbo/README.md and README.ru.md.");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("limbo_parse 0.10.0 (C99 syntax API)");
        return 0;
    }
    if (argc != 2)
    {
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (!input)
    {
        perror(argv[1]);
        return 2;
    }
    ok = limbo_read(input, &diagnostic);
    if (fclose(input))
    {
        return 2;
    }
    if (!ok)
    {
        fprintf(stderr,
                "%s:%d:%d: %s E%d (byte %ld)\n",
                argv[1],
                diagnostic.line,
                diagnostic.column,
                diagnostic.message,
                diagnostic.code,
                diagnostic.offset);
    }
    return ok ? 0 : 1;
}
