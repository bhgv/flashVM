/* Byte snapshots and local REAL predicate; no managed ownership. */
#ifndef DVM_REAL_BRANCH_VALUE_H
#define DVM_REAL_BRANCH_VALUE_H


/* source/other borrow jointly guarded REAL cells; pair is owner-local storage
 * for two REAL representations. Copy bytes only; no FP evaluation, allocation,
 * callback or ownership transfer. Caller supplies complete mapped spans. */
void dvm_runtime_real_branch_capture(const void *source, const void *other, void *pair);


/* condition selects EQ/NE/LT/LE/GT/GE (0..5); pair borrows two saved REAL byte
 * representations. Return predicate using the current FP environment; may raise
 * its normal FP exceptions. Caller MUST hold no object guards. Unknown internal
 * condition aborts. No shared read/store, allocation, callback or owner change. */
int dvm_runtime_real_branch_predicate(int condition, const void *pair);
#endif
