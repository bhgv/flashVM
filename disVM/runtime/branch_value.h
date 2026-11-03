/* Pure captured integer predicates and guarded target publication. */
#ifndef DVM_BRANCH_VALUE_H
#define DVM_BRANCH_VALUE_H


/* opcode selects a validated integer comparison; source/other are live jointly
 * guarded cells of that width. Return its boolean predicate after both snapshots.
 * No store, callback, allocation, FP operation or owner transfer. Unknown op aborts. */
int dvm_runtime_branch_predicate(int opcode, const void *source, const void *other);


/* target is a valid guarded native pointer slot; pc is current owner-local PC.
 * Return1 after publishing a non-nil/non-H snapshot,0 without publication for
 * those sentinels. No instruction/code dereference or lifetime proof is supplied. */
int dvm_runtime_branch_target(const void *target, void *pc);
#endif
