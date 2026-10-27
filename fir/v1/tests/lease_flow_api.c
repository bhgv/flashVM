/* Exercise retained ownership verification through the public C99 library API. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Parse a balanced alias scope, then forge type-correct but invalid ownership.
 * Own all modules/streams; return nonzero with a diagnostic on failure. Backends
 * must reject each mutation without publishing bytes to the borrowed output. */
int main(void)
{
    FirModule *base = malloc(sizeof(*base));
    FirModule *trial = malloc(sizeof(*trial));
    FILE *input = fir_temporary();
    FILE *output = fir_temporary();
    char error[256] = "";
    int variant;
    int ok = 0;
    if (!base || !trial || !input || !output)
    {
        goto done;
    }
    fputs("FIR 1.0\nmodule @leases\nfunc @main() -> void {\n^entry:\n"
          "%t: table.big = table.new\n%u: table.big = table.new\n"
          "%alias: table.big = copy %t\ntable.lock %t\ntable.unlock %alias\nreturn\n}\n", input);
    rewind(input);
    if (!fir_parse(input, base, error, sizeof(error)) || !fir_verify(base, error, sizeof(error)))
    {
        goto done;
    }
    if (fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (variant = 0; variant < 5; variant++)
    {
        memcpy(trial, base, sizeof(*trial));
        switch (variant)
        {
        case 0:
            trial->instructions[4].a = 1; /* Unlock a distinct, unowned table. */
            break;
        case 1:
            trial->instructions[4].removed = 1; /* Tombstoned leave leaks at return. */
            break;
        case 2:
            trial->instructions[3].removed = 1; /* Tombstoned entry cannot authorize leave. */
            break;
        case 3:
            trial->instructions[3].op = O_TABLE_UNLOCK; /* Two leaves do not form a scope. */
            break;
        default:
            trial->instructions[3].a = 1; /* Correct types do not imply matching owner identity. */
            break;
        }
        if (fir_verify(trial, error, sizeof(error)) || fir_optimise(trial, error, sizeof(error)) ||
            fir_dis(output, trial, error, sizeof(error)) || ftell(output) != 3 ||
            fir_object(output, trial, error, sizeof(error)) || ftell(output) != 3)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "ownership C99 verification failed: %s\n", error);
    }
    if (input)
    {
        fclose(input);
    }
    if (output)
    {
        fclose(output);
    }
    free(trial);
    free(base);
    return !ok;
}
