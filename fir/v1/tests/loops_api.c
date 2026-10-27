/* Check executable loops through the reusable compiler/IDE C99 library path. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>

/* Compile one valid loop or one invalid staged header step.
 * Borrow reusable module storage; own streams and diagnostics. Return one only
 * when parsing, cyclic verification and staged publication meet their contracts. */
static int compile_loop(FirModule *module, int invalid)
{
    FILE *input = fir_temporary();
    FILE *fir = fir_temporary();
    FILE *debug = fir_temporary();
    FILE *image = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char error[256] = "private stream allocation failed";
    int ok = 0;
    int compiled;
    if (!input || !fir || !debug || !image)
    {
        goto done;
    }
    fputs("implement M; M: module {value: fn(): big;}; value(): big {", input);
    fputs(invalid == 1 ? "sum := big 0; for (i := big 0; i < 3; extra := 42) {sum += i;} return sum;"
          : invalid == 2
              ? "sum := big 0; for (i := big 0; i < 3; i++"
              : "t := table of big {20}; old := t; i := int 0; while (i < int 1) {t = table of big {22}; i++;} "
                "s := \"a\"; while (len s < int 2) {s = s+\"😀\";} keys := keysof t; key := keys[0]; "
                "i = int 0; while (i < int 1) {key = keys[0]; i++;} return old[0]+t[key];",
          input);
    fputs("}", input);
    rewind(input);
    if (invalid)
    {
        fputs("OLD", fir);
        fputs("OLD", debug);
    }
    compiled = fir_source_debug(input, fir, FIR_LIMBO, "loop_service", error, sizeof(error), &diagnostic, debug);
    if (invalid)
    {
        ok = !compiled &&
             (invalid == 1 ? strstr(error, "step cannot declare") != NULL : diagnostic.kind == FIR_DIAG_SYNTAX) &&
             ftell(fir) == 3 && ftell(debug) == 3;
    }
    else
    {
        rewind(fir);
        ok = compiled && fir_parse(fir, module, error, sizeof(error)) && fir_verify(module, error, sizeof(error)) &&
             !fir_acyclic(module, error, sizeof(error)) && fir_optimise(module, error, sizeof(error)) &&
             fir_dis(image, module, error, sizeof(error)) && ftell(image) > 0;
    }
done:
    if (!ok)
    {
        fprintf(stderr, "loop compiler API failed: %s\n", error);
    }
    if (input)
    {
        fclose(input);
    }
    if (fir)
    {
        fclose(fir);
    }
    if (debug)
    {
        fclose(debug);
    }
    if (image)
    {
        fclose(image);
    }
    return ok;
}

/* Alternate valid/semantic-error/valid/syntax-error/valid requests without generated-parser state
 * leaking between calls. Own the module; return nonzero with explicit failures. */
int main(void)
{
    FirModule *module = malloc(sizeof(*module));
    int round;
    if (!module)
    {
        return 1;
    }
    for (round = 0; round < 5; round++)
    {
        if (!compile_loop(module, round == 1 ? 1 : round == 3 ? 2 : 0))
        {
            free(module);
            return 1;
        }
    }
    free(module);
    puts("PASS C99 loop compiler service: cyclic Dis and failed-request isolation");
    return 0;
}
