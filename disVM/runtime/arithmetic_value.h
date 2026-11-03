/* Internal unsigned integer arithmetic; caller jointly guards all three spans. */
#ifndef DVM_ARITHMETIC_VALUE_H
#define DVM_ARITHMETIC_VALUE_H


/* opcode selects admitted ADD/SUB/AND/OR/XOR B/W/L; first/second/output are live
 * possibly unaligned/overlapping cells of that width. Capture both before store.
 * Return void; unknown opcode aborts. No allocation, VM callback or FP work. */
void dvm_runtime_arithmetic_value(int opcode, const void *first, const void *second, void *output);
#endif
