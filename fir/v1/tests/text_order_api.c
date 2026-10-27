/* Public C99 IR signatures and effects for ordered strings must match Coco/R input. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* No arguments; return zero only after every check succeeds. Own one parsed
 * fixture and a sentinel output stream. Check forged operand and
 * result types before either backend can publish bytes; close all local owners. */
int main(void)
{
    FirModule *base = malloc(sizeof(*base));
    FirModule *module = malloc(sizeof(*module));
    FILE *input = fir_temporary();
    FILE *output = fir_temporary();
    char error[256] = "";
    int ok = 0;
    int variant;
    if (!base || !module || !input || !output)
    {
        goto done;
    }
    fputs("FIR 1.0\nmodule @text\nfunc @main() -> void {\n^entry:\n"
          "%a: string = const.string \"4\"\n%b: string = const.string \"2\"\n"
          "%n: i32 = const.i32 1\n%c: bool = string.lt %a, %b\n"
          "%same: bool = string.lt %a, %b\ntrace %same\nreturn\n}\n",
          input);
    rewind(input);
    if (!fir_parse(input, base, error, sizeof(error)) || !fir_verify(base, error, sizeof(error)) ||
        fir_effects(O_STRING_LT) != FIR_READS_MEMORY || fir_effects(O_STRING_EQ) != FIR_READS_MEMORY ||
        fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (variant = 0; variant < 6; ++variant)
    {
        int index = variant < 3 ? 3 : 4;
        FirInstruction *in;
        memcpy(module, base, sizeof(*module));
        in = &module->instructions[index];
        if (variant % 3 == 0)
        {
            in->a = 2; /* A valid int identity is still an invalid string operand. */
        }
        else if (variant % 3 == 1)
        {
            in->b = 2;
        }
        else
        {
            in->type = T_I32;
            module->values[in->result].type = T_I32;
        }
        if (fir_verify(module, error, sizeof(error)) || !strstr(error, "operand/result type mismatch") ||
            fir_dis(output, module, error, sizeof(error)) || ftell(output) != 3 ||
            fir_object(output, module, error, sizeof(error)) || ftell(output) != 3)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "ordered string C99 API checkpoint failed: %s\n", error);
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
    free(base);
    return ok ? 0 : 1;
}
