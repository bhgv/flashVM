/* Verify the public append-only bool IDs, type signatures and staged backend rejection. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* No arguments; return zero only after pure effects and all forged-signature
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
    if (O_BOOL_NOT != O_I32_XOR + 1 || O_BOOL_EQ != O_BOOL_NOT + 1 ||
        fir_effects((FirOp)(O_BOOL_EQ + 1)) != FIR_ALL_EFFECTS)
    {
        goto done;
    }
    if (fputs("FIR 1.0\nmodule @bits\nfunc @main() -> void {\n^entry:\n"
              "%a: bool = const.bool true\n%b: bool = const.bool false\n%f: i32 = const.i32 0\n"
              "%r: bool = bool.not %a\ntrace %r\nreturn\n}\n",
              input) < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, base, error, sizeof(error)) || fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (op = O_BOOL_NOT; op <= O_BOOL_EQ; op++)
    {
        base->instructions[3].op = (FirOp)op;
        base->instructions[3].b = op == O_BOOL_EQ ? 1 : -1;
        if (fir_effects((FirOp)op) != 0 || !fir_verify(base, error, sizeof(error)))
        {
            goto done;
        }
        for (variant = 0; variant < (op == O_BOOL_EQ ? 3 : 2); variant++)
        {
            FirInstruction *in;
            memcpy(module, base, sizeof(*module));
            in = &module->instructions[3];
            if (variant == 0)
            {
                in->a = 2; /* An int is not an implicit bool input. */
            }
            else if (variant == 1 && op == O_BOOL_EQ)
            {
                in->b = 2;
            }
            else
            {
                in->type = T_I32;
                module->values[in->result].type = T_I32;
            }
            if (fir_verify(module, error, sizeof(error)) || !strstr(error, "operand/result type mismatch") ||
                fir_dis(output, module, error, sizeof(error)) || fir_object(output, module, error, sizeof(error)) ||
                ftell(output) != 3)
            {
                goto done;
            }
        }
    }
    /* Arity and normalized constants are separate from signature admission. */
    memcpy(module, base, sizeof(*module));
    module->instructions[3].b = -1;
    if (fir_verify(module, error, sizeof(error)) || fir_dis(output, module, error, sizeof(error)) || ftell(output) != 3)
    {
        goto done;
    }
    memcpy(module, base, sizeof(*module));
    module->instructions[0].integer = 2;
    if (fir_verify(module, error, sizeof(error)) || fir_object(output, module, error, sizeof(error)) ||
        ftell(output) != 3)
    {
        goto done;
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "typed bool API failed: %s\n", error);
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
