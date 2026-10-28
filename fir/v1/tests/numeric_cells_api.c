/* Public verifier/backend checks for append-only private numeric cell operations. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Check one borrowed scalar kind and its opcode family. Return success after
 * canonical roundtrip and raw/optimized emission, then reject forged signatures
 * without writing either backend stream. Own all temporary IR/streams locally;
 * calls are synchronous and touch no VM, scheduler or native resource owner. */
static int exercise(const char *kind, FirType cell, FirOp local, FirOp store, FirOp load)
{
    FirModule *base = malloc(sizeof(*base));
    FirModule *module = malloc(sizeof(*module));
    FILE *input = fir_temporary();
    FILE *printed = fir_temporary();
    FILE *output = fir_temporary();
    char error[256] = "";
    int variant;
    int ok = 0;
    if (!base || !module || !input || !printed || !output)
    {
        goto done;
    }
    if (fprintf(input,
                "FIR 1.0\nmodule @api\nfunc @main() -> void {\n^entry:\n"
                "%%cell: %s.slot = %s.local\n%%value: %s = const.%s 1\n"
                "%%wrong: bool = const.bool true\n%s.store %%cell, %%value\n"
                "%%read: %s = %s.load %%cell\ntrace %%read\nreturn\n}\n",
                kind,
                kind,
                kind,
                kind,
                kind,
                kind,
                kind) < 0 ||
        fseek(input, 0, SEEK_SET) || !fir_parse(input, base, error, sizeof(error)) ||
        !fir_verify(base, error, sizeof(error)) || !fir_print(printed, base) || fseek(printed, 0, SEEK_SET) ||
        !fir_parse(printed, module, error, sizeof(error)) || base->instructions[0].type != cell ||
        base->instructions[0].op != local || base->instructions[3].op != store || base->instructions[4].op != load ||
        fir_effects(local) != FIR_WRITES_MEMORY || fir_effects(store) != FIR_WRITES_MEMORY ||
        fir_effects(load) != FIR_READS_MEMORY || !fir_dis(output, module, error, sizeof(error)) ||
        !fir_object(output, module, error, sizeof(error)) || !fir_optimise(module, error, sizeof(error)) ||
        !fir_dis(output, module, error, sizeof(error)) || !fir_object(output, module, error, sizeof(error)))
    {
        goto done;
    }
    fclose(output);
    output = fir_temporary();
    if (!output || fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (variant = 0; variant < 7; ++variant)
    {
        memcpy(module, base, sizeof(*module));
        if (variant == 0)
        {
            module->instructions[0].type = T_BOOL_SLOT;
            module->values[0].type = T_BOOL_SLOT;
        }
        else if (variant == 1)
        {
            module->instructions[0].a = 1;
        }
        else if (variant == 2)
        {
            module->instructions[3].b = 2;
        }
        else if (variant == 3)
        {
            module->instructions[4].a = 1;
        }
        else if (variant == 4)
        {
            module->instructions[4].b = 2;
        }
        else if (variant == 5)
        {
            module->instructions[5].a = 0;
        }
        else
        {
            module->instructions[4].op = O_COPY;
        }
        if (fir_verify(module, error, sizeof(error)) || !error[0] || fir_dis(output, module, error, sizeof(error)) ||
            fir_object(output, module, error, sizeof(error)) || ftell(output) != 3)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "FAIL numeric cell API %s: %s\n", kind, error);
    }
    if (input)
    {
        fclose(input);
    }
    if (printed)
    {
        fclose(printed);
    }
    if (output)
    {
        fclose(output);
    }
    free(module);
    free(base);
    return ok;
}


/* No arguments. Return zero only for preserved public IDs, conservative unknown
 * effects and successful i32/f64 library checks with seven rejections per kind. */
int main(void)
{
    if (T_I32_SLOT != T_KEY_CELL + 1 || T_F64_SLOT != T_I32_SLOT + 1 || O_I32_LOCAL != O_F64_LE + 1 ||
        O_F64_LOAD != O_I32_LOCAL + 5 || fir_effects((FirOp)(O_F64_LOAD + 1)) != FIR_ALL_EFFECTS ||
        !exercise("i32", T_I32_SLOT, O_I32_LOCAL, O_I32_STORE, O_I32_LOAD) ||
        !exercise("f64", T_F64_SLOT, O_F64_LOCAL, O_F64_STORE, O_F64_LOAD))
    {
        return 1;
    }
    puts("PASS private numeric cells: IDs, roundtrip, effects, 14 signatures and staged backends");
    return 0;
}
