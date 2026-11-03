/* Reuse retained aggregate opcodes, checked counts and C marking policy. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "owner_fast.h"
#include "aggregate_fast.h"

extern void movmp(void); /* Retained aggregate opcode, process-live C text. */

extern void headmp(void); /* Retained aggregate opcode, process-live C text. */

extern void consmp(void); /* Retained aggregate opcode, process-live C text. */

extern void consm(void); /* Retained aggregate opcode, process-live C text. */


/* [GC:] registers borrows decoded rooted state under existing admission.
 * Refuse before operands; C movmp retains its ordinary error/ownership policy.
 * Tail scaffold does not read cache/module state after calling the opcode. */
int dvm_runtime_aggregate_movmp_fast(REG *registers)
{
    if (!dvm_runtime_owner_entry_ready(registers))
    {
        return 0;
    }
    movmp();
    return 1;
}


/* [GC:] registers borrows decoded rooted state under existing admission.
 * Refuse before operands; C headmp retains its ordinary error/ownership policy.
 * Tail scaffold does not read cache/module state after calling the opcode. */
int dvm_runtime_aggregate_headmp_fast(REG *registers)
{
    if (!dvm_runtime_owner_entry_ready(registers))
    {
        return 0;
    }
    headmp();
    return 1;
}


/* [GC:] registers borrows decoded rooted state under existing admission.
 * Refuse before operands; C consmp retains its ordinary error/ownership policy.
 * Tail scaffold does not read cache/module state after calling the opcode. */
int dvm_runtime_aggregate_consmp_fast(REG *registers)
{
    if (!dvm_runtime_owner_entry_ready(registers))
    {
        return 0;
    }
    consmp();
    return 1;
}


/* [GC:] registers borrows decoded rooted state under existing admission.
 * Refuse before operands; C consm retains its ordinary error/ownership policy.
 * Tail scaffold does not read cache/module state after calling the opcode. */
int dvm_runtime_aggregate_consm_fast(REG *registers)
{
    if (!dvm_runtime_owner_entry_ready(registers))
    {
        return 0;
    }
    consm();
    return 1;
}
