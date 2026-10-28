/* Verify the public append-only bit IDs, type signatures and staged backend rejection. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* No arguments; return zero only after pure effects and all nine forged-signature
 * cases pass. Own temporary IR/streams here; no caller data or native VM is accessed. */
int main(void)
{
    FirModule *base = malloc(sizeof(*base));
    FirModule *module = malloc(sizeof(*module));
    FILE *input = fir_temporary();
    FILE *output = fir_temporary();
    char error[256] = "";
    int op;
    int variant;
    int ok = 0;
    if (!base || !module || !input || !output)
    {
        goto done;
    }
    if (O_I32_AND != O_AS_MEMBER_SET + 1 || O_I32_OR != O_I32_AND + 1 || O_I32_XOR != O_I32_OR + 1 ||
        fir_effects((FirOp)(O_F64_LOAD + 1)) != FIR_ALL_EFFECTS)
    {
        goto done;
    }
    if (fputs("FIR 1.0\nmodule @bits\nfunc @main() -> void {\n^entry:\n"
              "%a: i32 = const.i32 -1\n%b: i32 = const.i32 42\n%f: f64 = const.f64 0.5\n"
              "%r: i32 = i32.and %a, %b\ntrace %r\nreturn\n}\n",
              input) < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, base, error, sizeof(error)) || fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (op = O_I32_AND; op <= O_I32_XOR; op++)
    {
        base->instructions[3].op = (FirOp)op;
        if (fir_effects((FirOp)op) != 0 || !fir_verify(base, error, sizeof(error)))
        {
            goto done;
        }
        for (variant = 0; variant < 3; variant++)
        {
            FirInstruction *in;
            memcpy(module, base, sizeof(*module));
            in = &module->instructions[3];
            if (variant == 0)
            {
                in->a = 2; /* A floating value is not an implicit int input. */
            }
            else if (variant == 1)
            {
                in->b = 2;
            }
            else
            {
                in->type = T_BOOL;
                module->values[in->result].type = T_BOOL;
            }
            if (fir_verify(module, error, sizeof(error)) || !strstr(error, "operand/result type mismatch") ||
                fir_dis(output, module, error, sizeof(error)) || fir_object(output, module, error, sizeof(error)) ||
                ftell(output) != 3)
            {
                goto done;
            }
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "signed32 bit API failed: %s\n", error);
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
