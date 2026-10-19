/* CLI for the Coco/R src1.atg teaching adapters. Generated parser globals
 * live only in this executable. fir1 owns the different FIR grammar globals. */
#include "src1p.h"
#include "lower.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern int fir_utf8(const unsigned char *, size_t);
extern Error_Func Custom_Error;
/* Report generated syntax errors with source coordinates and byte offset. */
static void report(int code, int line, int column, long offset)
{
    fprintf(stderr, "source:%d:%d: syntax E%d (byte %ld)\n", line, column, code, offset);
}
/* Select the tutorial language, parse once and publish only successful FIR. */
extern FILE *fir_temporary(void);
int main(int argc, char **argv)
{
    FILE *input, *temporary, *output;
    unsigned char *text;
    size_t length;
    int ok;
    if (argc != 5 || (strcmp(argv[1], "as1") && strcmp(argv[1], "as2") &&
        strcmp(argv[1], "as3") && strcmp(argv[1], "limbo")) || strcmp(argv[2], "--input")) {
        fprintf(stderr, "usage: fir1_source as1|as2|as3|limbo --input SOURCE OUTPUT.fir\n"); return 2;
    }
    if (!strcmp(argv[3], argv[4])) { fprintf(stderr, "different paths required\n"); return 2; }
    input = fopen(argv[3], "rb");
    if (!input) { perror(argv[3]); return 2; }
    text = (unsigned char *)calloc(65538, 1); temporary = fir_temporary();
    if (!text || !temporary) { free(text); if (temporary) fclose(temporary); fclose(input); return 2; }
    length = fread(text, 1, 65537, input);
    ok = !ferror(input) && length <= 65536 && fir_utf8(text, length);
    if (fclose(input)) ok = 0;
    if (!ok) { fprintf(stderr, "read error, invalid UTF-8/NUL or source exceeds 64 KiB\n"); goto done; }
    source_config(temporary, !strcmp(argv[1], "limbo") ? 4 : argv[1][2] - '0'); Custom_Error = report;
    fprintf(temporary, "FIR 1.0\nmodule @tutorial\n// Experimental %s numeric tutorial profile.\nfunc @main() -> void {\n^entry:\n", argv[1]);
    Parse(text, (int)length);
    fputs("    return\n}\n", temporary);
    ok = Successful() && !source_failed() && !ferror(temporary);
    if (ok && (fflush(temporary) || fseek(temporary, 0, SEEK_SET))) ok = 0;
    if (ok) {
        output = fopen(argv[4], "wb");
        if (!output) { perror(argv[4]); ok = 0; }
        else {
            while ((length = fread(text, 1, 65536, temporary)) != 0)
                if (fwrite(text, 1, length, output) != length) { ok = 0; break; }
            if (ferror(temporary)) ok = 0;
            if (fclose(output)) ok = 0;
        }
    }
done:
    fclose(temporary); free(text);
    return ok ? 0 : 1;
}
