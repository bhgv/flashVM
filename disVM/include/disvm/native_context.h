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
#ifdef __cplusplus
}
#endif
#endif
