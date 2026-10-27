/* Public C99 callers must not bypass UTF-8 validation by constructing IR directly. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>

/* Check strict UTF-8 before any target bytes are published.
 * No arguments. This serialized parser client owns its module and streams;
 * all paths close/free them and return zero only after every check succeeds. */
int main(void)
{
    static const unsigned char bad[][8] = {
        {0x80, 0},                               /* Lone continuation byte. */
        {0xc0, 0xaf, 0},                         /* Overlong ASCII. */
        {0xed, 0xa0, 0x80, 0},                   /* Surrogate scalar. */
        {0xf4, 0x90, 0x80, 0x80, 0},             /* Greater than U+10FFFF. */
        {0xf0, 0x9f, 0x98, 0}                    /* Truncated non-BMP scalar. */
    };
    FirModule *module = malloc(sizeof(*module)); /* Caller-owned public IR storage. */
    FILE *input = fir_temporary();               /* Private validated textual fixture. */
    FILE *output = fir_temporary();              /* Sentinel stream must remain untouched on rejection. */
    char error[256] = "";                        /* Copied verifier/backend diagnostic. */
    unsigned i;
    int ok = 0;
    if (!module || !input || !output)
    {
        goto done;
    }
    if (fputs("FIR 1.0\nmodule @strings\nfunc @main() -> void {\n^entry:\n"
              "%s: string = const.string \"ok\"\n%n: i32 = string.len %s\ntrace %n\nreturn\n}\n",
              input) < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, module, error, sizeof(error)) ||
        !fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    if (module->instructions[0].op != O_STRING || fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
    {
        memset(module->instructions[0].text, 0, FIR_TEXT);
        memcpy(module->instructions[0].text, bad[i], sizeof(bad[i]));
        if (fir_verify(module, error, sizeof(error)) || !strstr(error, "UTF-8") ||
            fir_dis(output, module, error, sizeof(error)) || ftell(output) != 3 ||
            fir_object(output, module, error, sizeof(error)) || ftell(output) != 3)
        {
            fprintf(stderr, "forged UTF-8 constant bypassed verifier/target preflight: %u\n", i);
            goto done;
        }
    }
    strcpy(module->instructions[0].text, "ok");
    if (!fir_verify(module, error, sizeof(error)))
    {
        goto done;
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "string C99 API checkpoint failed: %s\n", error);
    }
    if (input && fclose(input))
    {
        ok = 0;
    }
    if (output && fclose(output))
    {
        ok = 0;
    }
    free(module);
    return ok ? 0 : 1;
}
