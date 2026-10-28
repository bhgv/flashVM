/* Verify the public append-only binary64 comparison IDs, type signatures and staged backend rejection. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>


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
    if (O_F64_EQ != O_BOOL_EQ + 1 || O_F64_LT != O_F64_EQ + 1 || O_F64_LE != O_F64_LT + 1 ||
        fir_effects((FirOp)(O_F64_LE + 1)) != FIR_ALL_EFFECTS)
    {
        goto done;
    }
    if (fputs("FIR 1.0 // C99 API annotated header\nmodule @bits\nfunc @main() -> void {\n^entry:\n"
              "%a: f64 = const.f64 -1.0\n%b: f64 = const.f64 42.0\n%f: i32 = const.i32 1\n"
              "%r: bool = f64.eq %a, %b\ntrace %r\nreturn\n}\n",
              input) < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, base, error, sizeof(error)) || fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (op = O_F64_EQ; op <= O_F64_LE; op++)
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
                in->a = 2; /* An integer is not an implicit real input. */
            }
            else if (variant == 1)
            {
                in->b = 2;
            }
            else
            {
                in->type = T_F64;
                module->values[in->result].type = T_F64;
            }
            if (fir_verify(module, error, sizeof(error)) || !strstr(error, "operand/result type mismatch") ||
                fir_dis(output, module, error, sizeof(error)) || fir_object(output, module, error, sizeof(error)) ||
                ftell(output) != 3)
            {
                goto done;
            }
        }
    }
    /* Nonfinite inputs may arise at runtime but are never valid literal payloads. */
    {
        const double payloads[3] = {NAN, HUGE_VAL, -HUGE_VAL};
        for (variant = 0; variant < 3; variant++)
        {
            memcpy(module, base, sizeof(*module));
            module->instructions[0].real = payloads[variant];
            if (fir_verify(module, error, sizeof(error)) || fir_dis(output, module, error, sizeof(error)) ||
                fir_object(output, module, error, sizeof(error)) || ftell(output) != 3)
            {
                goto done;
            }
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "binary64 comparison API failed: %s\n", error);
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
