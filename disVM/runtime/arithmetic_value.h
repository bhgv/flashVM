/* Internal native integer arithmetic; caller jointly guards all three spans. */
#ifndef DVM_ARITHMETIC_VALUE_H
#define DVM_ARITHMETIC_VALUE_H


/* opcode selects admitted ADD/SUB/MUL/DIV/MOD/AND/OR/XOR B/W/L; first/second/output are live
 * possibly unaligned/overlapping cells of that width. Capture both before store.
 * DIV/MOD require nonzero captured divisor; signed minimum/-1 is handled.
 * Return void; unknown opcode aborts. No allocation, VM callback or FP work. */
void dvm_runtime_arithmetic_value(int opcode, const void *first, const void *second, void *output);


/* Read only the valid guarded divisor for DIV/MOD B/W/L. Return1 iff zero;
 * unknown opcode aborts. No allocation, other operand read or VM/FP callback. */
int dvm_runtime_divisor_zero(int opcode, const void *second);
#endif
