/* One explicit subcommand per operation keeps verification separate from
 * bytecode emission. Output is opened only after the input verifies. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>
/* Validate CLI arguments, run the selected pipeline, and stage output on success. */
extern FILE *fir_temporary(void);
int main(int argc, char **argv)
{
    FirModule *module;
    FILE *input, *output;
    char error[512] = "operation failed";
    int ok, check, optimise, compile, raw;
    if (argc < 3) goto usage;
    check = !strcmp(argv[1], "check");
    optimise = !strcmp(argv[1], "opt");
    compile = !strcmp(argv[1], "swf");
    raw = !strcmp(argv[1], "actions");
    if ((!check && !optimise && !compile && !raw && strcmp(argv[1], "print")) ||
        argc != (check ? 3 : 4)) goto usage;
    if (!check && !strcmp(argv[2], argv[3])) {
        fprintf(stderr, "input and output must be different paths\n"); return 2;
    }
    module = (FirModule *)malloc(sizeof(*module));
    if (!module) return 2;
    input = fopen(argv[2], "rb");
    if (!input) { perror(argv[2]); free(module); return 2; }
    ok = fir_parse(input, module, error, sizeof(error));
    if (fclose(input)) ok = 0;
    if (ok) ok = fir_verify(module, error, sizeof(error));
    if (ok && optimise) ok = fir_optimise(module, error, sizeof(error));
    if (!ok) { fprintf(stderr, "%s: %s\n", argv[2], error); free(module); return 1; }
    if (check) { free(module); return 0; }
    /* tmpfile also protects against aliases/hard links to the input file. */
    output = fir_temporary();
    if (!output) { perror("temporary output"); free(module); return 2; }
    ok = compile || raw ? fir_swf(output, module, raw, error, sizeof(error)) : fir_print(output, module);
    if (ok && (fflush(output) || fseek(output, 0, SEEK_SET))) ok = 0;
    if (ok) {
        FILE *destination = fopen(argv[3], "wb");
        if (!destination) ok = 0;
        else {
            unsigned char bytes[4096]; size_t count;
            while ((count = fread(bytes, 1, sizeof(bytes), output)) != 0)
                if (fwrite(bytes, 1, count, destination) != count) { ok = 0; break; }
            if (ferror(output)) ok = 0;
            if (fclose(destination)) ok = 0;
        }
    }
    if (fclose(output)) ok = 0;
    free(module);
    if (!ok) fprintf(stderr, "%s\n", error);
    return ok ? 0 : 2;
usage:
    fprintf(stderr, "usage: fir1 check INPUT\n       fir1 print|opt|swf|actions INPUT OUTPUT\n");
    return 2;
}
