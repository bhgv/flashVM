/* Owner-local FP evaluation and guarded byte publication. */
#ifndef DVM_REAL_ARITHMETIC_VALUE_H
#define DVM_REAL_ARITHMETIC_VALUE_H


/* opcode selects validated ADDF/SUBF/MULF/DIVF; pair borrows two saved REAL
 * representations, result owns sizeof(REAL) local bytes. Evaluate native FP
 * outside ALL guards, preserving its native exception/rounding policy. Return
 * void; unknown internal opcode aborts. No shared read/owner/allocator callback. */
void dvm_runtime_real_arithmetic_value(int opcode, const void *pair, void *result);
#endif
