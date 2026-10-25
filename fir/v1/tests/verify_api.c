/* Reject invalid caller-constructed IR indices before any table dereference. */
#include "fir.h"
#include <stdlib.h>
#include <string.h>


/* Reject forged indices and physical block layouts through the public verifier.
 * No arguments; successful completion frees private modules and closes the fixture stream. */
int main(void)
{
    FirModule *base = (FirModule *)malloc(sizeof(*base)), *m = (FirModule *)malloc(sizeof(*m));
    FILE *in = fir_temporary();
    char error[256];
    int i;
    if (!base || !m || !in)
    {
        return 1;
    }
    fputs("FIR 1.0\nmodule @safe\nfunc @main() -> void { ^entry: %n: i32 = const.i32 42 trace %n return }\n", in);
    rewind(in);
    if (!fir_parse(in, base, error, sizeof(error)) || !fir_verify(base, error, sizeof(error)))
    {
        return 2;
    }
    fclose(in);
    for (i = 0; i < 15; ++i)
    {
        memcpy(m, base, sizeof(*m));
        switch (i)
        {
        case 0:
            m->count = FIR_LIMIT + 1;
            break;
        case 1:
            m->value_count = -1;
            break;
        case 2:
            m->instructions[0].block = FIR_BLOCKS;
            break;
        case 3:
            m->instructions[1].a = FIR_LIMIT;
            break;
        case 4:
            m->instructions[2].op = O_JUMP;
            break;
        case 5:
            m->values[0].definition = m->count;
            break;
        case 6:
            memset(m->module, 'a', FIR_TEXT);
            break;
        case 7:
            m->instructions[0].result = -1;
            break;
        case 8:
            m->instructions[2].yes = 0;
            break;
        case 9:
            m->instructions[0].op = (FirOp)999;
            break;
        case 10:
            m->instructions[1].b = 0;
            break;
        case 12:
            m->source_language = FIR_AS3;
            m->source_semantics = FIR_LIMBO_I32;
            break;
        case 11:
            strcpy(m->module, "@bad-name");
            break;
        case 13:
        case 14:
            /* Logical entry->other is reachable, but its physical instruction layout would
             * execute the wrong block in both emitters. Removed prefixes must not hide entry. */
            m->block_count = 2;
            strcpy(m->blocks[1].name, "^other");
            m->blocks[1].defined = 1;
            m->instructions[1].op = O_RETURN;
            m->instructions[1].block = 1;
            m->instructions[1].a = -1;
            m->instructions[2].op = O_JUMP;
            m->instructions[2].yes = 1;
            if (i == 14)
            {
                m->instructions[0].removed = 1;
            }
            break;
        }
        if (fir_verify(m, error, sizeof(error)))
        {
            return 10 + i;
        }
    }
    if (fir_verify(NULL, error, sizeof(error)))
    {
        return 30;
    }
    free(base);
    free(m);
    return 0;
}
