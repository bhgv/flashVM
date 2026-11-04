/* Borrowed snapshots for guarded native string comparisons. */
#ifndef DVM_STRING_BRANCH_VALUE_H
#define DVM_STRING_BRANCH_VALUE_H
#include <stddef.h>
#include <stdint.h>


/* source is a complete guarded pointer cell. Return borrowed pointer bits;
 * no dereference, ownership change, allocation, callback or wait. */
void *dvm_runtime_string_branch_source(const void *source);


/* object is an independently live owner whose len/max span is guarded.
 * length/capacity are owner-local outputs. Snapshot both int fields; no payload
 * access, owner change, allocation, callback, validation or wait. */
void dvm_runtime_string_branch_header(const void *object, int *length, int *capacity);


/* data borrows a guarded common-prefix payload; index is in that prefix and wide
 * selects Rune rather than byte representation. Return complete uint32 key bits;
 * no validation, allocation, callback, mutation or ownership transfer. */
uint32_t dvm_runtime_string_branch_key(const void *data, size_t index, int wide);
#endif
