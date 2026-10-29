/* Binary emission must match a consumer of the service's serialized FIR. */
#include "compiler_engine.h"
#include "fir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Compare one language/options pair using independent public FIR parse/emission.
 * source is borrowed NUL-terminated input. Return zero with a named diagnostic
 * on failure; dispose all API buffers, streams and the large private IR owner. */
static int compare(int language, int optimize, const char *source)
{
    DvmCompileOutput text;
    DvmCompileOutput binary;
    FirModule *module;
    FILE *input;
    FILE *output;
    char error[512];
    size_t size;
    size_t index;
    int ok = 0;
    dvm_compile(language, 2, optimize, (const unsigned char *)source, strlen(source), &text);
    dvm_compile(language, 1, optimize, (const unsigned char *)source, strlen(source), &binary);
    module = (FirModule *)malloc(sizeof(*module));
    input = fir_temporary();
    output = fir_temporary();
    if (text.stage != DVM_COMPILE_OK || binary.stage != DVM_COMPILE_OK || !module || !input || !output)
    {
        goto done;
    }
    size = text.artifact_size;
    if (fwrite(text.artifact, 1, size, input) != size || fseek(input, 0, SEEK_SET) ||
        !fir_parse(input, module, error, sizeof(error)) || !fir_verify(module, error, sizeof(error)) ||
        !fir_dis(output, module, error, sizeof(error)) || fseek(output, 0, SEEK_SET))
    {
        goto done;
    }
    for (index = 0; index < binary.artifact_size; ++index)
    {
        if (fgetc(output) != binary.artifact[index])
        {
            goto done;
        }
    }
    ok = fgetc(output) == EOF && !ferror(output);
done:
    if (!ok)
    {
        fprintf(stderr, "FAIL canonical compiler layout language=%d optimize=%d\n", language, optimize);
    }
    if (input)
    {
        fclose(input);
    }
    if (output)
    {
        fclose(output);
    }
    free(module);
    dvm_compile_dispose(&text);
    dvm_compile_dispose(&binary);
    return ok;
}


/* Check constant-folded removed slots for four languages and both option values.
 * No arguments/VM/subprocesses. Nonzero exit identifies a public binary mismatch. */
int main(void)
{
    int language;
    int optimize;
    for (language = 1; language <= 4; ++language)
    {
        const char *source = language == 4 ? "implement M;M:module{value:fn():int;};value():int{return 6*7;}"
                                          : "trace(6*7);";
        for (optimize = 0; optimize <= 1; ++optimize)
        {
            if (!compare(language, optimize, source))
            {
                return 1;
            }
        }
    }
    puts("PASS canonical compiler layout: four languages, both options, exact Dis bytes");
    return 0;
}
