/* Check executable loops through the reusable compiler/IDE C99 library path. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Compile valid scoped function flow or invalid staged header/join initialization.
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
    long body_offset; /* UTF-8 offset before the staged body, used to verify deferred diagnostics. */
    if (!input || !fir || !debug || !image)
    {
        goto done;
    }
    fputs("implement M; M: module {value: fn(): big;}; value(): big {", input);
    body_offset = ftell(input);
    fputs(invalid == 5   ? "for (item in table of big {42}"
          : invalid == 4 ? "x: string; for (i := int 0; i < int 2; x = x+\"2\") {i++;} return 42;"
          : invalid == 3 ? "x: big; if (1 == 1) {x = 42;} return x;"
          : invalid == 1 ? "sum := big 0; for (i := big 0; i < 3; extra := 42) {sum += i;} return sum;"
          : invalid == 2
              ? "sum := big 0; for (i := big 0; i < 3; i++"
              : "t: table of big; old := table of big {20}; i := int 0; "
                "for (amount in old) {old[0] = amount;} "
                "if (i == int 0) {t = table of big {22};} else {t = table of big {7};} "
                "s := \"a\"; while (len s < int 2) {s = s+\"😀\";} keys := keysof t; key := keys[0]; "
                "i = int 0; while (i < int 1) {key = keys[0]; i++;} "
                "scratch: string; for (j := int 0; j < int 3; j++) {scratch = \"42\"; t[key] = big scratch;} "
                "for (j := int 0; j < int 1; scratch = scratch+\"x\") {scratch = \"22\"; j++;} "
                "scratch = \"22\"; t[key] = big scratch; return old[0]+t[key];",
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
             (invalid == 4   ? strstr(error, "for step") != NULL
              : invalid == 1 ? strstr(error, "step cannot declare") != NULL
              : invalid == 3 ? strstr(error, "initialized") != NULL
                             : diagnostic.kind == FIR_DIAG_SYNTAX) &&
             ftell(fir) == 3 && ftell(debug) == 3;
        if (invalid == 4)
        {
            ok =
                ok && diagnostic.kind == FIR_DIAG_SEMANTIC && diagnostic.has_location &&
                diagnostic.byte_offset == (size_t)body_offset + strlen("x: string; for (i := int 0; i < int 2; x = ") &&
                diagnostic.byte_length == 1;
        }
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


/* Alternate valid requests with header semantic/syntax errors and a joined-path error without parser state
 * leaking between calls. Own the module; return nonzero with explicit failures. */
int main(void)
{
    FirModule *module = malloc(sizeof(*module));
    int round;
    if (!module)
    {
        return 1;
    }
    for (round = 0; round < 11; round++)
    {
        if (!compile_loop(module,
                          round == 1   ? 1
                          : round == 3 ? 2
                          : round == 5 ? 3
                          : round == 7 ? 4
                          : round == 9 ? 5
                                       : 0))
        {
            free(module);
            return 1;
        }
    }
    free(module);
    puts("PASS C99 loop compiler service: cyclic Dis and failed-request isolation");
    return 0;
}
