/* Pure guarded string-length payload publication; native input widths only. */
#ifndef DVM_STRING_LENGTH_VALUE_H
#define DVM_STRING_LENGTH_VALUE_H


/* source borrows a guarded pointer slot. Return its captured borrowed pointer,
 * without acquiring an owner/root, allocation, callback or referent access. */
void *dvm_runtime_string_length_source(const void *source);


/* field borrows a guarded native int or is NULL for H string; destination
 * borrows a guarded writable WORD. Capture before store, preserving aliases.
 * Return0 for INT_MIN without publishing,1 after success; no allocation, FP, ownership transfer, VM callback or root proof. */
int dvm_runtime_string_length_value(const void *field, void *destination);
#endif
