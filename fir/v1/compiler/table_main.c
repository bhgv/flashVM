/* CLI for explicit reference execution; it never claims to compile or run Dis/SWF. */
#include "fir_table.h"
#include <stdlib.h>

/* Read/verify one FIR file and execute the bounded table subset, reporting staged traces on success. */
int main(int argc, char **argv)
{
    FirModule *module;
    FILE *input;
    char error[512] = "table execution failed";
    int ok;
    if (argc != 2)
    {
        fprintf(stderr, "usage: fir1_table_run INPUT.fir (hosted reference execution, no VM/JIT)\n");
        return 2;
    }
    input = fopen(argv[1], "rb");
    module = malloc(sizeof(*module));
    if (!input || !module)
    {
        if (input)
        {
            fclose(input);
        }
        free(module);
        return 2;
    }
    ok = fir_parse(input, module, error, sizeof(error));
    fclose(input);
    if (ok)
    {
        ok = fir_table_run(module, stdout, error, sizeof(error));
    }
    free(module);
    if (ok && fflush(stdout))
    {
        snprintf(error, sizeof(error), "table trace flush failure");
        ok = 0;
    }
    if (!ok)
    {
        fprintf(stderr, "%s\n", error);
    }
    return ok ? 0 : 1;
}
