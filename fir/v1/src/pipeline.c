/* In-process FIR control; CLI and IDE use precisely this pipeline. */
#include "fir.h"
#include <stdlib.h>


int fir_run(FirTask task, FILE *input, FILE *output, const FirRunOptions *options, char *error, size_t capacity)
{
    FirModule *module;
    int ok, optimise;
    if (error && capacity)
    {
        error[0] = 0;
    }
    if (!input || task < FIR_CHECK || task > FIR_OBJECT || (task != FIR_CHECK && !output))
    {
        if (error && capacity)
        {
            snprintf(error, capacity, "invalid FIR pipeline arguments");
        }
        return 0;
    }
    if (!error)
    {
        capacity = 0;
    }
    module = (FirModule *)malloc(sizeof(*module));
    if (!module)
    {
        if (error && capacity)
        {
            snprintf(error, capacity, "out of memory");
        }
        return 0;
    }
    ok = fir_parse(input, module, error, capacity) && fir_verify(module, error, capacity);
    optimise = task == FIR_OPT || (options && options->optimise);
    if (ok && options && options->snapshot)
    {
        ok = options->snapshot(options->user, "before", module);
    }
    if (ok && optimise)
    {
        ok = fir_optimise(module, error, capacity);
    }
    if (ok && options && options->snapshot)
    {
        ok = options->snapshot(options->user, "after", module);
    }
    if (ok && task != FIR_CHECK)
    {
        ok = task == FIR_OBJECT ? fir_object(output, module, error, capacity) : fir_print(output, module);
    }
    if (!ok && options && options->snapshot && error && capacity && !error[0])
    {
        snprintf(error, capacity, "pipeline callback/output rejected");
    }
    free(module);
    return ok;
}
