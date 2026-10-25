/* Verify source-profile reset and transactional streams in a long-lived IDE compiler process. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Compile borrowed text in one profile; expected zero requires rejection and untouched sentinels.
 * module is reusable caller-owned IR storage. All private streams close on every return path.
 * Parser calls remain serialized by main. Returns one for the expected complete behavior. */
static int check(const char *text, FirLanguage language, FirSemantics expected, FirModule *module)
{
    FILE *source = fir_temporary();
    FILE *output = fir_temporary();
    FILE *debug = fir_temporary();
    FILE *dis = fir_temporary();
    FirSourceDiagnostic diagnostic;
    char message[256] = "";
    int accepted;
    int ok = 0;
    if (!source || !output || !debug || !dis)
    {
        goto done;
    }
    if (fputs(text, source) < 0 || fseek(source, 0, SEEK_SET))
    {
        goto done;
    }
    if (!expected && (fputs("old FIR", output) < 0 || fputs("old debug", debug) < 0))
    {
        goto done;
    }
    accepted = fir_source_debug(source, output, language, "profile_api", message, sizeof(message), &diagnostic, debug);
    if (!expected)
    {
        ok = !accepted && diagnostic.kind == FIR_DIAG_SEMANTIC && ftell(output) == 7 && ftell(debug) == 9;
    }
    else if (accepted && diagnostic.kind == FIR_DIAG_NONE && ftell(debug) > 0 &&
             !fflush(output) && !fseek(output, 0, SEEK_SET) && fir_parse(output, module, message, sizeof(message)) &&
             module->source_semantics == expected && fir_dis(dis, module, message, sizeof(message)))
    {
        ok = ftell(dis) > 0;
    }
done:
    if (source && fclose(source))
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
    if (!ok)
    {
        fprintf(stderr, "source profile API check failed: %s\n", message);
    }
    return ok;
}


/* Alternate real/int/AS successes and semantic failures repeatedly with one reusable IR owner.
 * No arguments; returns zero after every source, debug and Dis stream has been retired. */
int main(void)
{
    const char *real = "implement A; A: module {value: fn(): real;}; value(): real {return 7.5 / 2.0;}";
    const char *bad = "implement A; A: module {value: fn(): real;}; value(): int {return 2;}";
    const char *integer = "implement A; A: module {value: fn(): int;}; value(): int {return 42;}";
    FirModule *module = malloc(sizeof(*module));
    int cycle;
    int ok = module != NULL;
    for (cycle = 0; cycle < 8 && ok; cycle++)
    {
        ok = check(real, FIR_LIMBO, FIR_LIMBO_REAL, module) &&
             check(bad, FIR_LIMBO, FIR_SEM_UNSPECIFIED, module) &&
             check(integer, FIR_LIMBO, FIR_LIMBO_I32, module) &&
             check("trace(2.5);", FIR_AS3, FIR_AVM1_NUMERIC, module);
    }
    free(module);
    if (ok)
    {
        puts("PASS C99 source API: eight real/failure/int/AS profile resets and transactional debug/FIR streams");
    }
    return ok ? 0 : 1;
}
