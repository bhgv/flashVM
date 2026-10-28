/* Exercise numeric lazy conditions through public source and FIR libraries. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Compile borrowed source text through public C99 APIs. Return success only
 * after parsing, verification, optional optimization and both backend emissions.
 * Own and retire all staging streams/IR locally; calls must be serialized by
 * the embedding host. Error text is copied to stderr, with no VM/thread launch. */
static int compile_case(const char *text, int optimise)
{
    FirModule *module = malloc(sizeof(*module));
    FILE *source = fir_temporary();
    FILE *fir = fir_temporary();
    FILE *debug = fir_temporary();
    FILE *object = fir_temporary();
    FILE *dis = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char error[256] = "";
    int ok = 0;
    if (!module || !source || !fir || !debug || !object || !dis)
    {
        snprintf(error, sizeof(error), "cannot allocate API fixture staging");
        goto done;
    }
    if (fputs(text, source) < 0 || fseek(source, 0, SEEK_SET) ||
        !fir_source_debug(source, fir, FIR_LIMBO, "numeric_logic", error, sizeof(error), &diagnostic, debug) ||
        diagnostic.kind != FIR_DIAG_NONE || fseek(fir, 0, SEEK_SET) || !fir_parse(fir, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    if (optimise && !fir_optimise(module, error, sizeof(error)))
    {
        goto done;
    }
    if (!fir_verify(module, error, sizeof(error)) || !fir_object(object, module, error, sizeof(error)) ||
        !fir_dis(dis, module, error, sizeof(error)) || ftell(object) <= 24 || ftell(dis) <= 0)
    {
        goto done;
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "FAIL numeric logic API: %s\n", error);
    }
    if (source)
    {
        fclose(source);
    }
    if (fir)
    {
        fclose(fir);
    }
    if (debug)
    {
        fclose(debug);
    }
    if (object)
    {
        fclose(object);
    }
    if (dis)
    {
        fclose(dis);
    }
    free(module);
    return ok;
}


/* Reject borrowed source text without changing either caller-owned output.
 * Return success only for a located semantic diagnostic and unchanged stream
 * positions. Own/close the local fixtures; parser state must reset on next call. */
static int reject_case(const char *text)
{
    FILE *source = fir_temporary();
    FILE *fir = fir_temporary();
    FILE *debug = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char error[256] = "";
    int ok = 0;
    if (!source || !fir || !debug)
    {
        goto done;
    }
    if (fputs(text, source) < 0 || fseek(source, 0, SEEK_SET) || fputs("OLD FIR", fir) < 0 ||
        fputs("OLD DEBUG", debug) < 0)
    {
        goto done;
    }
    if (fir_source_debug(source, fir, FIR_LIMBO, "numeric_logic", error, sizeof(error), &diagnostic, debug) ||
        diagnostic.kind != FIR_DIAG_SEMANTIC || !diagnostic.has_location || !diagnostic.byte_length ||
        ftell(fir) != 7 || ftell(debug) != 9)
    {
        goto done;
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "FAIL numeric expression rejection/rollback: %s\n", error);
    }
    if (source)
    {
        fclose(source);
    }
    if (fir)
    {
        fclose(fir);
    }
    if (debug)
    {
        fclose(debug);
    }
    return ok;
}


/* No arguments. Return zero after repeated int/real compilations prove reset
 * of private parser/cell state and borrowed debug stream retirement. Actual
 * execution and independent truth-table oracles belong to numeric_logic.py. */
int main(void)
{
    int cycle;
    for (cycle = 0; cycle < 8; ++cycle)
    {
        if (!reject_case("implement M; M: module {value: fn(): int;}; value(): int {"
                         "return +(1 < 2);}") ||
            !reject_case("implement M; M: module {value: fn(): real;}; value(): real {"
                         "x := (1.0 < 2.0); return 42.5;}") ||
            !reject_case("implement M; M: module {value: fn(): int;}; value(): int {"
                         "if((1 < 2) < (3 < 4)){return 42;}else{return -7;}}") ||
            !compile_case("implement M; M: module {value: fn(): real;}; value(): real {"
                          "if((!(0.0/0.0 <= 1.0) || 4.0 == 5.0) && !!(2.0 < 3.0))"
                          "{return 42.5;}else{return -7.25;}}",
                          cycle % 2) ||
            !compile_case("implement M; M: module {value: fn(): int;}; value(): int {"
                          "if(((1 == 1) == (2 < 3)) && (7 + 2 == 0 || !(2 > 3)))"
                          "{return 42;}else{return -7;}}",
                          cycle % 2))
        {
            return 1;
        }
    }
    puts("PASS numeric logic public source/FIR libraries: 16 compilations and 24 transactional rejections");
    return 0;
}
