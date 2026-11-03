/* Internal integer conversion primitive; caller supplies guarded live spans. */
#ifndef DVM_INTEGER_VALUE_H
#define DVM_INTEGER_VALUE_H


/* Convert one admitted opcode from source to destination, capturing before store.
 * Operands may be unaligned/overlap. No allocation, callback, ownership or FP work.
 * Unsupported opcodes abort (caller must validate first). Returns void. */
void dvm_runtime_integer_value(int opcode, const void *source, void *destination);
#endif
