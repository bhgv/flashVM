/* C99 host for the optional public table reference API, independent of a source checkout. */
#include "fir_table.h"
#include <stdlib.h>

/* Read one FIR file, verify/execute the table subset and print staged traces.
 * argv[1] is a borrowed input path. This host owns/releases its module and stream on all paths.
 * Returns 0 on success, 1 on verification/runtime failure, 2 on argument/allocation/I/O setup errors.
 * No Dis interpreter, JIT, FlashVM heap or application callback is invoked. */
int main(int argc, char **argv)
{
    FirModule *module;
    FILE *input;
    char error[512] = "table execution failed";
    int ok;
    if (argc != 2)
    {
        fprintf(stderr, "usage: table-client INPUT.fir\n");
        return 2;
    }
    module = malloc(sizeof(*module));
    input = fopen(argv[1], "rb");
    if (!module || !input)
    {
        if (input)
        {
            fclose(input);
        }
        free(module);
        fprintf(stderr, "cannot allocate/read table input\n");
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
