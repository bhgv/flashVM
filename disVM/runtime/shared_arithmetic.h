/* Hosted three-span integer arithmetic, no managed ownership transfer. */
#ifndef DVM_SHARED_ARITHMETIC_H
#define DVM_SHARED_ARITHMETIC_H


/* [GC:] opcode selects ADD/SUB/MUL/DIV/MOD/AND/OR/XOR B/W/L. first/second/output are borrowed
 * live cells. Exact ACTIVE worker context and valid nonempty geometry required.
 * Return NULL after publication or static error before effects. One ordered union
 * guards capture/evaluation/publication. DIV/MOD preserve zero-divisor priority
 * over invalid dividend/output geometry, with errors returned only after unlock. No mapping/root proof or VM callback. */
const char *dvm_runtime_arithmetic(int opcode, const void *first, const void *second, void *output);


/* Same borrowed operands and diagnostic as the opcode adapter. retryable is
 * required caller-private storage. It receives one only if a BUSY claim
 * rolled back without reading operands or publishing the result. */
const char *dvm_runtime_arithmetic_attempt(int opcode, const void *first, const void *second,
    void *output, int *retryable);
#endif
