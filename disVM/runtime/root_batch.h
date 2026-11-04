/* Preallocated atomic root groups; registration never creates managed owners. */
#ifndef DVM_ROOT_BATCH_H
#define DVM_ROOT_BATCH_H
#include <stddef.h>

typedef struct DVM_RootBatch DVM_RootBatch; /* Opaque native owner-local metadata. */


/* Reserve capacity occurrences and initialize registry exclusion before locks.
 * Return NULL on native allocation/overflow/zero capacity; no roots published.
 * Caller owns stable batch and excludes concurrent operations on that batch.
 * Keys and managed references are not acquired by reservation. May allocate. */
DVM_RootBatch *dvm_runtime_root_batch_create(size_t capacity);


/* Publish count nonnull borrowed keys as one group to existing locked visitors.
 * count <= capacity, including zero; an active batch refuses without changes.
 * keys/storage stay valid through return; referents and independent owners stay
 * valid through clear. No allocation, callback, payload access or managed retain.
 * Registry mutex may wait. This API alone supplies no GC epoch/lock permission.
 * Hosted use needs existing ACTIVE/stopped-world publication exclusion. Ordinary
 * remove cannot consume these occurrences. Return 1 success, 0 unchanged refusal. */
int dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count);


/* Replace the complete active group in one registry-mutex interval. new_count
 * <= capacity, including zero; every nonempty key must be nonnull. Validate all
 * inputs before mutation. Return1 on success,0 unchanged on NULL/inactive/invalid
 * vector. Old sparse occurrences are discarded; new indices follow the new vector.
 * Caller exclusively owns the batch, independently keeps old/new referents and
 * strong owners alive through return, and supplies stable disjoint key storage.
 * No allocation/free, retain/release, payload read or callback. Mutex may wait.
 * New keys inherit publish lifetime obligations until withdrawn; old retirement
 * still needs caller GC/collector-lifetime exclusion after return.
 * This closes only registry visibility gaps; hosted use still needs ACTIVE/
 * stopped-world publication exclusion, correct lock order and GC epochs. */
int dvm_runtime_root_batch_replace(DVM_RootBatch *batch, void *const *keys, size_t new_count);


/* Withdraw one occurrence by its original published vector index. No allocation,
 * referent access or callback. Caller holds independent owner until its consuming
 * destructor; remaining occurrences stay rooted. Duplicate/invalid indices refuse
 * unchanged. An emptied group stays active until clear; same exclusive-owner and
 * GC/lock-order rules as publish. Return 1 removed, 0 unchanged refusal. */
int dvm_runtime_root_batch_remove(DVM_RootBatch *batch, size_t index);


/* Remove exactly this active group, atomically to visitors, without allocation,
 * callbacks or referent access. Return 0 unchanged for NULL/inactive; 1 clears
 * even a published empty group. Caller retains referents/owners through return.
 * Metadata mutex can wait; batch operations require external exclusive ownership. */
int dvm_runtime_root_batch_clear(DVM_RootBatch *batch);


/* Free inactive native reservation and zero *batch; active groups refuse.
 * Return 0 unchanged for NULL pointer/value or active. Caller externally quiesces
 * all uses before destroy. No referent release or registry shutdown occurs. */
int dvm_runtime_root_batch_destroy(DVM_RootBatch **batch);
#endif
