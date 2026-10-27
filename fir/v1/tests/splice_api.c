/* Check public typed-range signatures before either backend publishes output. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Parse valid capabilities and their copies, then forge operand/result types.
 * Own fixture modules and sentinel stream; failures must publish no bytes.
 * Return zero only after signatures, effects and preflight checks all pass. */
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
    fputs("FIR 1.0\nmodule @splice\nfunc @main() -> void {\n^entry:\n"
          "%tab: table.big = table.new\n%zero: i64 = const.i64 0\n"
          "%bounds: big.range = range.big %zero, %zero\n"
          "%slot: table.range = table.range %tab, %bounds\n"
          "%copy: table.range = copy %slot\ntable.splice %copy, %tab\n"
          "%text: string = const.string \"A\"\n%index: i32 = const.i32 0\n"
          "%char: string.range = string.char.range %text, %index\n"
          "%piece: string = rune.text %index\n"
          "%new: string = string.splice %char, %piece\n"
          "%span: string.range = string.range %text, %bounds\n"
          "%saved: string.range = copy %span\n"
          "%whole: string = string.splice %saved, %text\nreturn\n}\n",
          input);
    rewind(input);
    if (!fir_parse(input, base, error, sizeof(error)) || !fir_verify(base, error, sizeof(error)) ||
        fir_effects(O_TABLE_RANGE) != (FIR_READS_MEMORY | FIR_MAY_THROW) ||
        fir_effects(O_TABLE_SPLICE) != (FIR_READS_MEMORY | FIR_WRITES_MEMORY | FIR_ALLOCATES | FIR_MAY_THROW) ||
        fir_effects(O_STRING_SPLICE) != (FIR_READS_MEMORY | FIR_ALLOCATES | FIR_MAY_THROW) ||
        !fir_dis(output, base, error, sizeof(error)))
    {
        goto done;
    }
    fclose(output);
    output = fir_temporary();
    if (!output || fputs("OLD", output) < 0)
    {
        goto done;
    }
    for (variant = 0; variant < 9; variant++)
    {
        memcpy(module, base, sizeof(*module));
        switch (variant)
        {
        case 0:
            module->instructions[3].b = 1; /* Scalar is not a bounds pair. */
            break;
        case 1:
            module->instructions[5].b = 1; /* Replacement must be a table owner. */
            break;
        case 2:
            module->instructions[8].b = 1; /* Rune index must have int type. */
            break;
        case 3:
            module->instructions[9].a = 1; /* Rune conversion cannot accept big. */
            break;
        case 4:
            module->instructions[10].b = 0; /* String replacement cannot accept a table. */
            break;
        case 5:
            module->instructions[11].a = 0; /* A string range cannot capture a table. */
            break;
        case 6:
            module->instructions[13].a = 3; /* Table and string ranges are distinct capabilities. */
            break;
        case 7:
            module->instructions[4].type = T_TABLE_SLOT;
            module->values[4].type = T_TABLE_SLOT;
            break;
        default:
            module->instructions[12].type = T_TABLE_RANGE;
            module->values[11].type = T_TABLE_RANGE;
            break;
        }
        if (fir_verify(module, error, sizeof(error)) || fir_dis(output, module, error, sizeof(error)) ||
            ftell(output) != 3 || fir_object(output, module, error, sizeof(error)) || ftell(output) != 3)
        {
            goto done;
        }
    }
    ok = 1;
done:
    if (!ok)
    {
        fprintf(stderr, "typed splice API checkpoint failed: %s\n", error);
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
