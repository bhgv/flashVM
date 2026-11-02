/* Tutorial: audited native work with context-owned heap-call state. */
#ifndef DVM_EXAMPLE_NATIVE_WAIT_H
#define DVM_EXAMPLE_NATIVE_WAIT_H
#include "disvm/native_owner.h"
#ifdef __cplusplus
extern "C"
{
#endif

/* Read one native collector epoch while the caller owns the heap. user is
 * borrowed native metadata; return an unsigned wrapping epoch. No throwing,
 * lifecycle change, allocation or implicit native release is permitted. */
typedef unsigned long (*DVM_ExampleEpoch)(void *user);

/* Perform native work outside the heap. user borrows native-only buffers/handles
 * kept alive for the entire call. Return a scalar status even on failure;
 * do not touch managed memory, throw, cancel or migrate the native owner. */
typedef int (*DVM_ExampleOperation)(void *user);


/* Run an audited synchronous native operation while allowing other owners/GC
 * to progress. owner must be current/ACTIVE with complete published managed
 * roots. epoch/clock and operation/data borrow native storage through return;
 * status borrows a writable int and changes only after reentry. Return zero for
 * precondition failure, one for a completed round trip (including negative
 * operation status). Broken reentry aborts rather than returning without heap
 * ownership. This helper does not publish hidden managed C-stack roots. */
int dvm_example_native_wait(DVM_HeapOwner *owner, DVM_ExampleEpoch epoch,
    void *clock, DVM_ExampleOperation operation, void *data, int *status);
#ifdef __cplusplus
}
#endif
#endif
