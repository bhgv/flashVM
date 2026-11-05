/* Two-phase REAL arithmetic; existing hosted lifetime/admission stays required. */
#ifndef DVM_SHARED_REAL_ARITHMETIC_H
#define DVM_SHARED_REAL_ARITHMETIC_H


/* [GC:] opcode is ADDF/SUBF/MULF/DIVF/NEGF; NEGF uses only first/output.
 * Each used argument borrows independently
 * live mapped REAL spans, possibly unaligned/overlapping. Capture the used sources
 * under one union, release completely, evaluate native FP, then claim/publish
 * output. Two observation points, NOT an atomic shared read-modify-write.
 * Return static error/NULL only after complete unlock. No allocation, reference
 * transfer or VM callback under guards. Native FP faults occur without guards.
 * Caller retains ACTIVE/world, frame/referent lifetime and FP environment. */
const char *dvm_runtime_real_arithmetic(int opcode, const void *first,
    const void *second, void *output);
/* The original opcode evaluator receives only private, saved operands.
 * It returns its diagnostic after capture. Frame-reading variants run ACTIVE. */
const char *dvm_runtime_scaled_arithmetic(int opcode, const void *first, const void *second,
    void *output, const char *(*evaluate)(const void *, const void *, void *));
const char *dvm_runtime_fixed_product(int opcode, WORD scale, const void *first,
    const void *second, void *output, const char *(*evaluate)(WORD, const void *, const void *, void *));
#endif
