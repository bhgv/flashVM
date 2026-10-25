/* Embed direct Dis generation and verify that backend rejection preserves caller streams. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Parse one constant fixture into caller-owned module storage; all streams remain private here.
 * Returns zero on input, allocation or parse failure; error receives the parser diagnostic. */
static int parse(const char *text, FirModule *module, char *error, size_t capacity)
{
    FILE *input = fir_temporary();
    int ok = 0;
    if (!input)
    {
        return 0;
    }
    if (fputs(text, input) >= 0 && !fseek(input, 0, SEEK_SET))
    {
        ok = fir_parse(input, module, error, capacity);
    }
    if (fclose(input))
    {
        ok = 0;
    }
    return ok;
}


/* Check public API ownership/error recovery, including NULL diagnostics and invalid IR.
 * No arguments; all heap/stream owners are retired on both success and failure. */
int main(void)
{
    const char *valid = "FIR 1.0\nmodule @M\nfunc @main() -> void {\n^entry:\n"
                        "%x: i32 = const.i32 -2147483648\ntrace %x\nreturn\n}\n";
    const char *unsupported = "FIR 1.0\nmodule @M\nfunc @main() -> void {\n^entry:\n"
                              "%x: f64 = const.f64 2.5\ntrace %x\nreturn\n}\n";
    FirModule *module = malloc(sizeof(*module));
    FILE *output = fir_temporary();
    char error[256] = "";
    long initial;
    int ok = 0;
    if (!module || !output || fputs("sentinel", output) < 0)
    {
        goto done;
    }
    initial = ftell(output);
    if (initial != 8 || !parse(unsupported, module, error, sizeof(error)) ||
        fir_dis(output, module, error, sizeof(error)) || !strstr(error, "Dis i32 backend") || ftell(output) != initial)
    {
        goto done;
    }
    /* Failed coverage must not poison later compilation in the same IDE process. */
    if (!parse(valid, module, error, sizeof(error)) || !fir_dis(output, module, NULL, 0) || ftell(output) <= initial)
    {
        goto done;
    }
    initial = ftell(output);
    module->value_count = FIR_LIMIT + 1;
    if (fir_dis(output, module, NULL, sizeof(error)) || ftell(output) != initial || fir_dis(NULL, module, error, sizeof(error)) ||
        fir_dis(output, NULL, error, sizeof(error)))
    {
        goto done;
    }
    puts("PASS direct Dis C API: borrowed output, preflight, NULL diagnostics and sequential recovery");
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "Dis API check failed: %s\n", error);
    }
    if (output && fclose(output))
    {
        ok = 0;
    }
    free(module);
    return ok ? 0 : 1;
}
