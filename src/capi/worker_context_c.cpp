#include "../core/runtime.hpp"
#include "../core/worker_context.h"
extern "C" void fvm_context_bind_worker(FVM_Context *c,FVM_Worker *w) {if(c)c->worker=w;}
extern "C" FVM_Result fvm_worker_current(FVM_Context *c,FVM_Worker **out) {
    if(!c || !out)return FVM_INVALID_ARGUMENT;
    *out=c->worker;return c->worker ? FVM_OK : FVM_NOT_FOUND;
}
