/* Exercise constructor parser lifetime and staged publication in an IDE-style C99 client. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Compile mode 0 (valid), 1 (syntax error) or 2 (SSA limit) with caller-owned
 * streams. module is reusable borrowed IR storage, reinitialized by fir_parse.
 * Return one on expected behavior, zero with diagnostics on failure. Calls are
 * serialized and may block for compilation/I/O; no compiler subprocess is used. */
static int cycle(int mode, FirModule *module)
{
    FILE *input = fir_temporary();
    FILE *output = fir_temporary();
    FILE *debug = fir_temporary();
    FILE *dis = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char error[256] = "cannot allocate private streams";
    int ok = 0;
    int compiled;
    int i;
    if (!input || !output || !debug || !dis)
    {
        goto done;
    }
    fputs("implement M; M: module {value: fn(): big;}; value(): big {t := table of big {", input);
    if (mode == 0)
    {
        fputs("40, answer /* trivia */ = 42, 2,", input);
    }
    else if (mode == 1)
    {
        fputs("40,,2", input);
    }
    else
    {
        for (i = 0; i < 1500; ++i)
        {
            fputs(i ? ",1" : "1", input);
        }
    }
    fputs("}; return t.answer;}", input);
    rewind(input);
    if (mode)
    {
        fputs("old FIR", output);
        fputs("old debug", debug);
    }
    compiled =
        fir_source_debug(input, output, FIR_LIMBO, "constructor_service", error, sizeof(error), &diagnostic, debug);
    if (mode)
    {
        if (compiled || diagnostic.kind == FIR_DIAG_NONE || ftell(output) != 7 || ftell(debug) != 9 ||
            (mode == 2 && !strstr(error, "limit")))
        {
            goto done;
        }
    }
    else
    {
        if (!compiled || diagnostic.kind != FIR_DIAG_NONE || ftell(debug) <= 0)
        {
            goto done;
        }
        rewind(output);
        if (!fir_parse(output, module, error, sizeof(error)) || !fir_verify(module, error, sizeof(error)) ||
            !fir_dis(dis, module, error, sizeof(error)) || ftell(dis) <= 0)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "constructor C99 service cycle %d failed: %s\n", mode, error);
    }
    if (input && fclose(input))
    {
        ok = 0;
    }
    if (output && fclose(output))
    {
        ok = 0;
    }
    if (debug && fclose(debug))
    {
        ok = 0;
    }
    if (dis && fclose(dis))
    {
        ok = 0;
    }
    return ok;
}


/* No arguments; return zero after repeated success/error/limit/recovery calls.
 * The caller owns one IR buffer and retires it after all serialized invocations. */
int main(void)
{
    FirModule *module = malloc(sizeof(*module));
    int i;
    if (!module)
    {
        fputs("constructor service IR allocation failed\n", stderr);
        return 1;
    }
    for (i = 0; i < 13; ++i)
    {
        if (!cycle(i % 3, module))
        {
            free(module);
            return 1;
        }
    }
    free(module);
    puts("PASS constructor C99 service: repeated parse/debug/Dis and error recovery");
    return 0;
}
