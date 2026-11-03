/* Hosted SysV x64 guarded-copy tail target; include matching native interp.h. */
#ifndef DVM_BYTES_FAST_H
#define DVM_BYTES_FAST_H


/* [GC:] Refuse before operands for wrong REG/context/opcode or held guards. Normal completion
 * returns1; errors propagate without decline/retry after payload effects. Code
 * and return PC are process-live; no JIT mapping access follows a C callback. */
int dvm_runtime_bytes_fast(REG *registers, int opcode);
#endif
