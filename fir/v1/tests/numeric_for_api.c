/* Exercise numeric counted loops through public source and FIR libraries. */
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
        !fir_source_debug(source, fir, FIR_LIMBO, "numeric_for", error, sizeof(error), &diagnostic, debug) ||
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
        fprintf(stderr, "FAIL numeric for API: %s\n", error);
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
    if (fir_source_debug(source, fir, FIR_LIMBO, "numeric_for", error, sizeof(error), &diagnostic, debug) ||
        diagnostic.kind != FIR_DIAG_SEMANTIC || !diagnostic.has_location || !diagnostic.byte_length ||
        ftell(fir) != 7 || ftell(debug) != 9)
    {
        goto done;
    }
    {
        char saved_fir[8] = "";
        char saved_debug[10] = "";
        if (fseek(fir, 0, SEEK_SET) || fseek(debug, 0, SEEK_SET) || fread(saved_fir, 1, 7, fir) != 7 ||
            fread(saved_debug, 1, 9, debug) != 9 || strcmp(saved_fir, "OLD FIR") || strcmp(saved_debug, "OLD DEBUG"))
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "FAIL numeric for rejection/rollback: %s\n", error);
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


/* Interleave failed staged headers with valid int/real compilations. Return
 * zero only after public library reset, verifier and both backends succeed.
 * No arguments or runtime launch; numeric_flow.py owns execution oracles. */
int main(void)
{
    int cycle;
    char deep[2048] = "implement M;M:module{value:fn():int;};value():int{";
    for (cycle = 0; cycle < 70; ++cycle)
    {
        strcat(deep, "for(;;){");
    }
    strcat(deep, "break;");
    for (cycle = 0; cycle < 70; ++cycle)
    {
        strcat(deep, "}");
    }
    strcat(deep, "return 1;}");
    for (cycle = 0; cycle < 8; ++cycle)
    {
        if (!reject_case(deep) ||
            !reject_case("implement M;M:module{value:fn():int;};value():int{"
                         "for(i:=0;i<2;j:=0){continue;}return 1;}") ||
            !reject_case("implement M;M:module{value:fn():real;};value():real{"
                         "for(i:=0.0;i<2.0;i+=1.0){break;}return i;}") ||
            !reject_case("implement M;M:module{value:fn():int;};value():int{"
                         "for(i:=0;i<2;i++){continue;x:=1;}return 1;}") ||
            !compile_case("implement M;M:module{value:fn():int;};value():int{"
                          "sum:=0;for(i:=0;i<7;i++){i:=6;sum+=i;}return sum;}",
                          cycle % 2) ||
            !compile_case("implement M;M:module{value:fn():real;};value():real{"
                          "sum:=0.0;for(i:=0.0;i<7.0;i++){if(i==2.0){continue;}"
                          "sum+=7.0;}return sum;}",
                          cycle % 2))
        {
            return 1;
        }
    }
    puts("PASS numeric for public libraries: 16 compilations and 32 transactional rejections");
    return 0;
}
