/* Pure guarded array-length payload publication; native input widths only. */
#ifndef DVM_ARRAY_LENGTH_VALUE_H
#define DVM_ARRAY_LENGTH_VALUE_H


/* source borrows a guarded pointer slot. Return its captured borrowed pointer,
 * without acquiring an owner/root, allocation, callback or referent access. */
void *dvm_runtime_array_length_source(const void *source);


/* field borrows a guarded native WORD or is NULL for nil/H array; destination
 * borrows a guarded writable WORD. Capture before store, preserving aliases.
 * Return void; no allocation, FP, ownership transfer, VM callback or root proof. */
void dvm_runtime_array_length_value(const void *field, void *destination);
#endif
