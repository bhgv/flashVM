/* Continuous native string owner/header/payload exclusion. */
#ifndef DVM_SHARED_STRING_BRANCH_H
#define DVM_SHARED_STRING_BRANCH_H


/* [GC:] condition selects EQ/NE/LT/LE/GT/GE (0..5); source/other borrow live
 * pointer cells; target borrows a pointer cell only when the condition is true.
 * pc must be the exact current TLS PC. Return static error or NULL after unlock.
 * Caller proves root/referent/code lifetimes; guards do not grant provenance or
 * concurrent GC permission. No allocation, retain, callback or object-owner park.
 * Native metadata mutex fallback retains its existing contract. */
const char *dvm_runtime_string_branch(int condition, const void *source,
    const void *other, const void *target, void *pc);
extern int dvm_runtime_parallel_string; /* Startup-only bounded comparison experiment. */
#endif
