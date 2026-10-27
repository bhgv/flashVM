/* Diagnostic boundary for the retained hosted Inferno exception walker. */
#ifndef DVM_EXCEPTION_HOST_H
#define DVM_EXCEPTION_HOST_H



/* Copy the current bound owner's module path and predecessor PC for diagnostics.
 * registers must be its active REG; name is borrowed writable capacity bytes.
 * No heap allocation, ownership transfer or payload lock acquisition occurs. */
WORD dvm_exception_status(REG *registers, char *name, int capacity);
#endif
