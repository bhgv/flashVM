/* Optimize private test input with actual library plus one substituted pass. */
#include "fir.h"
#include <stdio.h>
#include <stdlib.h>


/* argv supplies private input/output paths, argc must be3. Own module/streams;
 * return0 only after verified optimization/publication. No oracle is embedded:
 * Python compares output semantics using the separate table evaluator. */
int main(int argc, char **argv)
{
    FirModule *module = malloc(sizeof(*module));
    FILE *input = NULL;
    FILE *output = NULL;
    char error[256] = "";
    int ok = 0;
    if (argc != 3 || !module)
    {
        goto done;
    }
    input = fopen(argv[1], "r");
    if (!input || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_optimise(module, error, sizeof(error)))
    {
        goto done;
    }
    output = fopen(argv[2], "w");
    if (!output || !fir_print(output, module))
    {
        goto done;
    }
    ok = 1;
done:
    if (input)
    {
        fclose(input);
    }
    if (output && fclose(output))
    {
        ok = 0;
    }
    free(module);
    if (!ok)
    {
        fprintf(stderr, "locality control optimizer: %s\n", error);
        return 1;
    }
    return 0;
}
