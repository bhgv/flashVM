/* Internal guarded native shift mechanics; include interp.h first. */
#ifndef DVM_SHIFT_VALUE_H
#define DVM_SHIFT_VALUE_H


/* opcode selects SHL/SHR B/W/L or LSR W/L. second is a valid guarded WORD cell;
 * count receives its captured value. Return1 iff in0..value_bits-1,0 otherwise.
 * No other operand read, allocation or VM callback. Unknown opcode aborts. */
int dvm_runtime_shift_count(int opcode, const void *second, WORD *count);


/* first/output are live jointly guarded cells; count is prevalidated/captured.
 * Return void after bit publication. No count re-read, callback or allocation.
 * Unknown opcode aborts; caller validates all operands before invoking this. */
void dvm_runtime_shift_value(int opcode, const void *first, WORD count, void *output);
#endif
