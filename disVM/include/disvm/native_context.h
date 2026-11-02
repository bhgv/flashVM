/* Experimental native-thread state: independent registers and current process. */
#ifndef DISVM_NATIVE_CONTEXT_H
#define DISVM_NATIVE_CONTEXT_H
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif
/* Opaque native allocation; never a VM/channel payload or a shared GC root. */
typedef struct DVM_NativeContext DVM_NativeContext;


/* Allocate zeroed register storage. The host owns any process/runtime pointers;
 * context borrows them and must be destroyed after unbinding and joining. */
DVM_NativeContext *dvm_native_context_create(size_t register_size, void *runtime);


/* Allocate one extended up object with embedded registers at a typed offsetof.
 * Caller supplies a real struct layout with correctly aligned register_offset.
 * process_offset identifies the pointer field holding current Prog (up.iprog).
 * The storage belongs to context, not a second TLS slot or a separate REG heap. */
DVM_NativeContext *dvm_native_context_create_embedded(
    size_t storage_size, size_t register_offset, size_t register_size, size_t process_offset, void *runtime);


/* Borrow the extended up object only on its bound native owner. */
void *dvm_native_context_host_process(DVM_NativeContext *context);


/* Destroy only a quiescent unbound context; returns0 for a bound/held-turn/guarded
 * context,1 on release. Caller must join users before destroying native storage. */
int dvm_native_context_destroy(DVM_NativeContext *context);


/* Bind exclusively to the current OS thread; nested or concurrent bind fails.
 * A successful binding must be paired with unbind before that thread exits. */
int dvm_native_context_bind(DVM_NativeContext *context);


/* Unbind only on the owning thread after poll/world/turn reservations retire.
 * Failure preserves binding and root/turn state, including during error cleanup. */
int dvm_native_context_unbind(DVM_NativeContext *context);


/* Borrow current native state; null means the thread has no Dis context. */
DVM_NativeContext *dvm_native_context_current(void);


/* Reserve one native scheduler turn on this bound owner. token is a borrowed
 * non-NULL native identity, not a managed root; caller retains its lifetime.
 * Refuse nested/foreign ownership. No allocation, waiting or process mutation. */
int dvm_native_context_acquire_turn(DVM_NativeContext *context, void *token);


/* End only this owner's exact turn token. Refuse a foreign/missing/wrong token
 * without mutation. Host must finish its native continuation first. No waiting. */
int dvm_native_context_release_turn(DVM_NativeContext *context, void *token);


/* Borrow this bound owner's turn token, or NULL for idle/foreign context.
 * Clearing the GC process slot does not clear this native lifetime reservation. */
void *dvm_native_context_turn(DVM_NativeContext *context);


/* Owner-thread accessors; pointers never migrate implicitly to another thread. */
void *dvm_native_context_registers(DVM_NativeContext *context);


/* Exact native register allocation extent; zero outside the owning thread. */
size_t dvm_native_context_register_size(DVM_NativeContext *context);


/* Borrow the runtime on its owner; caller retains it for the context lifetime. */
void *dvm_native_context_runtime(DVM_NativeContext *context);


/* Borrow the current process on its owner; unbind clears this field. */
void *dvm_native_context_process(DVM_NativeContext *context);


/* Publish an already rooted process on the owner; null explicitly clears it. */
int dvm_native_context_set_process(DVM_NativeContext *context, void *process);


/* Host-installed instruction-boundary hook; it may wait, but must not unwind.
 * One owner at a time. Setting a nonnull hook fails if one already exists. */
typedef int (*DVM_NativePoll)(void *user);


int dvm_native_context_set_poll(DVM_NativeContext *context, DVM_NativePoll poll, void *user);


/* Clear only the matching owner's hook; an installed hook prevents unbind. */
int dvm_native_context_clear_poll(DVM_NativeContext *context, DVM_NativePoll poll, void *user);


/* Call between complete instructions, never halfway through an instruction. */
int dvm_native_context_poll(void);
/* Guard-layer affinity accounting only. context must be current/bound.
 * Acquire refuses overflow; release refuses zero; no heap/root access occurs.
 * Exact guard ownership is checked separately by object_guard, not by this count. */
int dvm_native_context_object_acquire(DVM_NativeContext *context);


/* Consume one owner-local recorded guard; refuse foreign/unmatched calls. */
int dvm_native_context_object_release(DVM_NativeContext *context);


/* Return current owner's guard count, zero for foreign/unbound without reading
 * its mutable storage. Held guards forbid poll/turn release/unbind/world changes. */
unsigned long dvm_native_context_object_count(DVM_NativeContext *context);


/* Observe this owner's registered hook without invoking it. Foreign returns0;
 * this does not authorize access to registered roots or managed objects. */
int dvm_native_context_has_poll(DVM_NativeContext *context);


#ifdef __cplusplus
}
#endif
#endif
