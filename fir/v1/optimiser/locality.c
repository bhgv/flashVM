/* Conservative point-of-use ownership facts; no runtime Heap representation. */
#include "locality.h"
#include <string.h>


/* Identify only verifier-restricted frame cells. type copied; return1 for a
 * nonescaping variable HOME, not a promise about the object it references. */
static int frame_cell(FirType type)
{
    return type == T_BOOL_SLOT || type == T_BIG_SLOT || type == T_I32_SLOT ||
        type == T_F64_SLOT || type == T_STRING_CELL || type == T_TABLE_CELL ||
        type == T_KEYS_CELL || type == T_KEY_CELL;
}


/* Typed big-table operations have no metatable/application callback dispatch.
 * op is copied; return1 for every opaque/user operation. This does NOT narrow
 * global fir_effects: allocation, throwing, suspension and leases remain effects. */
int fir_locality_barrier(FirOp op)
{
    switch (op)
    {
    case O_REF_LOCAL:
    case O_REF_STORE:
    case O_REF_NIL:
    case O_BOOL_LOCAL:
    case O_BOOL_STORE:
    case O_BIG_LOCAL:
    case O_BIG_STORE:
    case O_I32_LOCAL:
    case O_I32_STORE:
    case O_F64_LOCAL:
    case O_F64_STORE:
    case O_TABLE_NEW:
    case O_TABLE_INDEX:
    case O_TABLE_LOAD:
    case O_TABLE_STORE:
    case O_TABLE_LEN:
    case O_TABLE_SLICE:
    case O_TABLE_KEYS:
    case O_TABLE_COUNT:
    case O_TABLE_HAS:
    case O_TABLE_DELETE:
    case O_TABLE_RANGE:
    case O_TABLE_SPLICE:
    case O_TABLE_LOCK:
    case O_TABLE_UNLOCK:
        return 0;
    default:
        return (fir_effects(op) & (FIR_MAY_CALL_USER | FIR_WRITES_MEMORY | FIR_MAY_SUSPEND)) != 0;
    }
}


/* Resolve immutable copied big constants independently of textual block order.
 * module/value borrowed and verified; output key changes only on success.
 * Negative/unknown keys refuse: relative-index normalization can change with
 * table length. No runtime table/key lookup or successful-execution claim. */
static int absolute_key(const FirModule *module, int value, int64_t *key)
{
    int remaining = module->value_count;
    while (value >= 0 && remaining--)
    {
        const FirInstruction *definition = &module->instructions[module->values[value].definition];
        if (definition->removed)
        {
            return 0;
        }
        if (definition->op == O_I64 && definition->big >= 0)
        {
            *key = definition->big;
            return 1;
        }
        if (definition->op != O_COPY || definition->type != T_I64)
        {
            return 0;
        }
        value = definition->a;
    }
    return 0;
}


/* Analyze verified live instructions into disjoint caller-owned facts. module
 * is borrowed and unchanged. Per-block maps track current reference-cell
 * contents, captured slot owners and immutable SSA snapshots separately. Every
 * block entry starts unknown; every opaque operation invalidates existing fresh
 * objects. Facts describe ONE instruction point, never an eternal private bit.
 * No allocation, execution, callbacks, waits or runtime ownership mutation. */
void fir_locality_facts(const FirModule *module, FirLocalityFacts *facts)
{
    int origin[FIR_LIMIT]; /* Current table allocation identity per value/cell. */
    int64_t key[FIR_LIMIT]; /* Captured absolute numeric key for each slot. */
    unsigned char known_key[FIR_LIMIT]; /* key is initialized and absolute. */
    unsigned char fresh[FIR_LIMIT]; /* Allocation has not crossed an opaque boundary. */
    unsigned depth[FIR_LIMIT]; /* Retained entries observed within this block only. */
    int block = -1;
    int index;
    memset(facts, 0, sizeof(*facts));
    for (index = 0; index < FIR_LIMIT; index++)
    {
        facts->table_owner[index] = -1;
    }
    for (index = 0; index < module->count; index++)
    {
        const FirInstruction *in = &module->instructions[index];
        int owner;
        if (in->removed)
        {
            continue;
        }
        if (in->block != block)
        {
            int value;
            block = in->block;
            for (value = 0; value < module->value_count; value++)
            {
                origin[value] = -1;
            }
            memset(known_key, 0, sizeof(known_key));
            memset(fresh, 0, sizeof(fresh));
            memset(depth, 0, sizeof(depth));
        }
        if (fir_locality_barrier(in->op))
        {
            memset(fresh, 0, sizeof(fresh));
        }
        owner = in->a < 0 ? -1 : origin[in->a];
        facts->table_owner[index] = owner;
        if (owner >= 0)
        {
            facts->private_table[index] = fresh[owner];
            facts->retained_table[index] = depth[owner] != 0;
        }
        if (in->a >= 0 && module->values[in->a].type == T_TABLE_SLOT && known_key[in->a])
        {
            facts->absolute_key[index] = 1;
            facts->numeric_key[index] = key[in->a];
        }
        if (in->result >= 0 && frame_cell(in->type))
        {
            facts->frame_local[in->result] = 1;
        }
        switch (in->op)
        {
        case O_TABLE_NEW:
        case O_TABLE_SLICE:
            origin[in->result] = in->result;
            fresh[in->result] = 1;
            depth[in->result] = 0;
            break;
        case O_COPY:
            origin[in->result] = owner;
            known_key[in->result] = in->a >= 0 ? known_key[in->a] : 0;
            if (known_key[in->result])
            {
                key[in->result] = key[in->a];
            }
            break;
        case O_TABLE_INDEX:
        case O_TABLE_RANGE:
            origin[in->result] = owner;
            if (in->op == O_TABLE_INDEX)
            {
                known_key[in->result] = absolute_key(module, in->b, &key[in->result]);
            }
            break;
        case O_REF_LOCAL:
        case O_REF_LOAD:
            if (in->type == T_TABLE_CELL || in->type == T_TABLE_BIG)
            {
                origin[in->result] = owner;
            }
            break;
        case O_REF_STORE:
            if (module->values[in->a].type == T_TABLE_CELL)
            {
                origin[in->a] = origin[in->b];
            }
            break;
        case O_REF_NIL:
            origin[in->result] = -1;
            break;
        case O_TABLE_LOCK:
            if (owner >= 0)
            {
                depth[owner]++;
            }
            break;
        case O_TABLE_UNLOCK:
            if (owner >= 0 && depth[owner])
            {
                depth[owner]--;
            }
            break;
        default:
            break;
        }
    }
}
