/* Guarded native integer branches; include interp.h first. */
#ifndef DVM_SHARED_BRANCH_H
#define DVM_SHARED_BRANCH_H


/* [GC:] opcode chooses one of eighteen existing comparisons. source/other/target
 * borrow live native slots; pc must be the current owner's REG.PC address. Return
 * static error or NULL after unlock. A false condition ignores invalid target
 * geometry/content. Caller owns mapping/root/code lifetime; no cross-owner PC,
 * callback, allocation or general parallel managed admission is supplied. */
const char *dvm_runtime_integer_branch(int opcode, const void *source,
    const void *other, const void *target, void *pc);
#endif
