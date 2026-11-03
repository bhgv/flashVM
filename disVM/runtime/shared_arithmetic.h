/* Hosted three-span integer arithmetic, no managed ownership transfer. */
#ifndef DVM_SHARED_ARITHMETIC_H
#define DVM_SHARED_ARITHMETIC_H


/* [GC:] opcode selects ADD/SUB/AND/OR/XOR B/W/L. first/second/output are borrowed
 * live cells. Exact ACTIVE worker context and valid nonempty geometry required.
 * Return NULL after publication or static error before effects. One ordered union
 * guards capture/evaluation/publication. No mapping/root proof or VM callback. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output);
#endif
