/* Consume compile-time locality facts without weakening explicit table leases. */
#include "locality.h"


/* Forward a successful store into a later load only for the same fresh private
 * owner, absolute constant key and an active retained table entry. module is
 * borrowed/verified; return changed flag. Mutations invalidate owner candidates,
 * unknown owners/opaque operations and CFG boundaries invalidate everything.
 * The original store/lease/allocation/key capture and their failures stay live.
 * Numeric retained lookup adds no allocation or new token; text/relative keys,
 * unleased reads and reference payload substitution are intentionally excluded. */
int fir_forward_private_tables(FirModule *module)
{
    FirLocalityFacts facts;
    int stored[FIR_LIMIT]; /* Immutable big SSA value, -1 means no candidate. */
    int64_t key[FIR_LIMIT]; /* Absolute numeric key paired with stored value. */
    int block = -1;
    int changed = 0;
    int index;
    fir_locality_facts(module, &facts);
    for (index = 0; index < module->count; index++)
    {
        FirInstruction *in = &module->instructions[index];
        int owner = facts.table_owner[index];
        if (in->removed)
        {
            continue;
        }
        if (block != in->block || fir_locality_barrier(in->op))
        {
            int value;
            block = in->block;
            for (value = 0; value < module->value_count; value++)
            {
                stored[value] = -1;
            }
        }
        if (in->op == O_TABLE_STORE || in->op == O_TABLE_DELETE || in->op == O_TABLE_SPLICE)
        {
            if (owner < 0)
            {
                int value;
                for (value = 0; value < module->value_count; value++)
                {
                    stored[value] = -1;
                }
            }
            else
            {
                stored[owner] = -1;
                if (in->op == O_TABLE_STORE && facts.private_table[index] && facts.absolute_key[index])
                {
                    stored[owner] = in->b;
                    key[owner] = facts.numeric_key[index];
                }
            }
        }
        else if (in->op == O_TABLE_LOAD && owner >= 0 && facts.private_table[index] &&
            facts.retained_table[index] && facts.absolute_key[index] && stored[owner] >= 0 &&
            key[owner] == facts.numeric_key[index])
        {
            in->op = O_COPY;
            in->a = stored[owner];
            in->b = -1;
            changed = 1;
        }
    }
    return changed;
}
