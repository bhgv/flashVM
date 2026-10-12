/* FlashVM: Interpreter state, object lifetimes and shared runtime helpers: worker context.
 * File: src/core/worker_context.h.
 */

#ifndef FVM_INTERNAL_WORKER_CONTEXT_H
/* Define FVM INTERNAL WORKER CONTEXT H constant or compilation switch. */
#define FVM_INTERNAL_WORKER_CONTEXT_H
#include "flashvm/worker.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Implement context bind worker for this subsystem. */
    void fvm_context_bind_worker(FVM_Context *, FVM_Worker *);
#ifdef __cplusplus
}
#endif
#endif
