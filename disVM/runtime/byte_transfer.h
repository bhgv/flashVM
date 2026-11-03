/* Bounded byte transfer primitive; no ownership/guard acquisition. */
#ifndef DVM_BYTE_TRANSFER_H
#define DVM_BYTE_TRANSFER_H
#include <stddef.h>


/* Copy already validated mapped spans with memmove overlap/bit semantics.
 * Caller owns lifetimes and either holds the complete shared range guards or
 * existing serial admission. No allocation, VM/error callback or owner effect. */
void dvm_runtime_byte_transfer(void *destination, const void *source, size_t size);
#endif
