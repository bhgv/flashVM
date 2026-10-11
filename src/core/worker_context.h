#ifndef FVM_INTERNAL_WORKER_CONTEXT_H
#define FVM_INTERNAL_WORKER_CONTEXT_H
#include "flashvm/worker.h"
#ifdef __cplusplus
extern "C" {
#endif
void fvm_context_bind_worker(FVM_Context *,FVM_Worker *);
#ifdef __cplusplus
}
#endif
#endif
