/* Continuous native string owner/header/payload exclusion. */
#ifndef DVM_SHARED_STRING_BRANCH_H
#define DVM_SHARED_STRING_BRANCH_H
#include <stddef.h>


/* [GC:] condition selects EQ/NE/LT/LE/GT/GE (0..5); source/other borrow live
 * pointer cells; target borrows a pointer cell only when the condition is true.
 * pc must be the exact current TLS PC. Return static error or NULL after unlock.
 * Caller proves root/referent/code lifetimes; guards do not grant provenance or
 * concurrent GC permission. No allocation, retain, callback or object-owner park.
 * Native metadata mutex fallback retains its existing contract. */
const char *dvm_runtime_string_branch(int condition, const void *source,
    const void *other, const void *target, void *pc);
extern int dvm_runtime_parallel_string; /* Startup-only bounded comparison experiment. */


/* Read a selected character with its source owner and destination guarded.
 * The decoded caller checks complete operand geometry before entry. */
const char *dvm_runtime_string_index(const void *source, const void *index_cell, void *destination);


/* Snapshot already validated cells into private output. A scheduled retry
 * sets replay and leaves all three snapshots untouched. No retain or store. */
const char *dvm_runtime_string_operands(int opcode, const void *first, const void *second,
    size_t input_size, const void *slot, void *saved_first, void *saved_second,
    void *old, int *replay);
#endif
