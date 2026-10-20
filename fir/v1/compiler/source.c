/* CLI for the Coco/R src1.atg teaching adapters. Generated parser globals
 * live only in this executable. fir1 owns the different FIR grammar globals. */
#include "fir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int fir_utf8(const unsigned char *, size_t);
/* Select the tutorial language, parse once and publish only successful FIR. */
extern FILE *fir_temporary(void);
int main(int argc, char **argv)
{
    FILE *input, *temporary, *output;
    unsigned char *text;
    size_t length;
    int ok;
    char error[512] = "source operation failed";
    if (argc == 2 && !strcmp(argv[1], "--version"))
    {
        puts("fir1_source 0.5.0 (numeric tutorial profiles)");
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        puts("usage: fir1_source as1|as2|as3|limbo --input SOURCE OUTPUT.fir [--module NAME]\n"
             "Numeric subset: literals, names, unary +/-, + - * /, parentheses.\n"
             "AS: var x = 1, y = .5; x += y; trace(x); AS3 const supported.\n"
             "AS2/AS3 also allow var x:Number = 1; AS1 rejects annotations.\n"
             "Limbo: value(): int tutorial with x: int = 1; or x:=16r2a; and return.\n"
             "One scope, initialized declarations, no implicit globals/hoisting.\n"
             "AS uses f64 including / and /=; Limbo i32 wrap rejects division.\n"
             "AS3 emits AVM1 compatibility, not ABC. FIR carries a source profile.\n"
             "Limits: 64 KiB input, 255-byte tokens, 256 names, 1000 SSA values,\n"
             "64 nested group/unary levels. Source syntax is compiler/src1.atg; lowering is C99.\n"
             "Exit: 0 success, 1 translation/output failure, 2 CLI/setup failure.\n"
             "See fir/v1/doc/COVERAGE.md and examples/ (also .ru.md).");
        return 0;
    }
    if ((argc != 5 && (argc != 7 || strcmp(argv[5], "--module"))) ||
        (strcmp(argv[1], "as1") && strcmp(argv[1], "as2") && strcmp(argv[1], "as3") &&
         strcmp(argv[1], "limbo")) ||
        strcmp(argv[2], "--input"))
    {
        fprintf(stderr,
                "usage: fir1_source as1|as2|as3|limbo --input SOURCE OUTPUT.fir [--module NAME]\n");
        return 2;
    }
    if (!strcmp(argv[3], argv[4]))
    {
        fprintf(stderr, "different paths required\n");
        return 2;
    }
    input = fopen(argv[3], "rb");
    if (!input)
    {
        perror(argv[3]);
        return 2;
    }
    text = (unsigned char *)calloc(65538, 1);
    temporary = fir_temporary();
    if (!text || !temporary)
    {
        free(text);
        if (temporary)
            fclose(temporary);
        fclose(input);
        return 2;
    }
    ok = fir_source(input, temporary,
                    (FirLanguage)(!strcmp(argv[1], "limbo") ? 4 : argv[1][2] - '0'),
                    argc == 7 ? argv[6] : NULL, error, sizeof(error));
    if (ok && (fflush(temporary) || fseek(temporary, 0, SEEK_SET)))
        ok = 0;
    if (ok)
    {
        output = fopen(argv[4], "wb");
        if (!output)
        {
            perror(argv[4]);
            ok = 0;
        }
        else
        {
            while ((length = fread(text, 1, 65536, temporary)) != 0)
                if (fwrite(text, 1, length, output) != length)
                {
                    ok = 0;
                    break;
                }
            if (ferror(temporary))
                ok = 0;
            if (fclose(output))
                ok = 0;
        }
    }
    fclose(input);
    fclose(temporary);
    free(text);
    if (!ok)
        fprintf(stderr, "%s\n", error[0] ? error : "source output/I/O failure");
    return ok ? 0 : 1;
}
