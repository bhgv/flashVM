/* Internal POSIX native error transfer, separate from managed/GC policy. */
#ifndef DVM_NATIVE_JUMP_H
#define DVM_NATIVE_JUMP_H
#include <setjmp.h>


/* buffer belongs to a still-live sigsetjmp on the current OS thread; value is
 * nonzero. Never return. Restore its signal mask after caller-owned cleanup.
 * No heap/root ownership, locking or cross-thread continuation is supplied. */
void dvm_native_error_jump(sigjmp_buf buffer, int value);
#endif
