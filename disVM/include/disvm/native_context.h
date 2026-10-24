/* Experimental native-thread state: independent registers and current process. */
#ifndef DISVM_NATIVE_CONTEXT_H
#define DISVM_NATIVE_CONTEXT_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
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
DVM_NativeContext *dvm_native_context_create_embedded(size_t storage_size, size_t register_offset,
                                                    size_t register_size, size_t process_offset, void *runtime);
/* Borrow the extended up object only on its bound native owner. */
void *dvm_native_context_host_process(DVM_NativeContext *context);
/* Destroy only an unbound context; returns 0 if still bound, 1 on release. */
int dvm_native_context_destroy(DVM_NativeContext *context);
/* Bind exclusively to the current OS thread; nested or concurrent bind fails.
 * A successful binding must be paired with unbind before that thread exits. */
int dvm_native_context_bind(DVM_NativeContext *context);
/* Unbind only on the owning thread. Fails without changing another owner. */
int dvm_native_context_unbind(DVM_NativeContext *context);
/* Borrow current native state; null means the thread has no Dis context. */
DVM_NativeContext *dvm_native_context_current(void);
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
#ifdef __cplusplus
}
#endif
#endif
