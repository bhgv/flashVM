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
/* Borrow a live string under NativeWorld and return its UTF-8 view. The
 * caller initializes problem to NULL. On invalid storage or allocation failure,
 * set a static diagnostic and return NULL without throwing through a guard. */
struct String;
char *dvm_runtime_string_view(struct String *string, const char **problem);

#endif
