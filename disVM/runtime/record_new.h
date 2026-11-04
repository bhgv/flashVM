/* Guarded record construction for the hosted NEW/NEWZ instructions. */
#ifndef DVM_RECORD_NEW_H
#define DVM_RECORD_NEW_H

struct Type; /* Immutable live descriptor owned by the caller. */


/* [GC:] Construct a fresh record and publish into a rooted owning destination.
 * clear selects scalar zeroing. Both modes initialize mapped fields to H.
 * Caller retains frame/cell/Type lifetime and ACTIVE NativeWorld admission.
 * Monitor may inspect or raise after rooting, and may write application slots
 * through the normal guarded APIs. It cannot retain the private candidate or
 * leave admission. Errors propagate after consuming cleanup. */
void dvm_runtime_record_new(void *destination, struct Type *type, int clear);
#endif
