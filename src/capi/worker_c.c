#include "flashvm/worker.h"
#include "../../ports/thread_port.h"
#include <stdlib.h>
#include <string.h>
struct FVM_Worker {
    FVM_PortThread *thread;
    FVM_Config config;
    FVM_WorkerFunction callback;
    void *userdata;
    FVM_Value input;
    FVM_Value output;
    FVM_Result result;
    int joined;
};
static FVM_Result copy_value(FVM_Value *dst, const FVM_Value *src) {
    size_t len;
    char *s;
    if (!dst || !src || src->type == FVM_VALUE_OBJECT) return FVM_INVALID_ARGUMENT;
    *dst = *src;
    if (src->type != FVM_VALUE_STRING) return FVM_OK;
    if (!src->string) return FVM_INVALID_ARGUMENT;
    len = strlen(src->string);
    s = (char *)malloc(len + 1);
    if (!s) return FVM_OUT_OF_MEMORY;
    memcpy(s, src->string, len + 1);
    dst->string = s;
    return FVM_OK;
}
static void free_value(FVM_Value *v) {
    if (v->type == FVM_VALUE_STRING) free((void *)v->string);
    v->string = 0;
}
static void worker_entry(void *arg) {
    FVM_Worker *w = (FVM_Worker *)arg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Value result;
    FVM_Result r;
    fvm_value_undefined(&result);
    r = fvm_runtime_create(&w->config, &runtime);
    if (r == FVM_OK) r = fvm_context_create(runtime, &context);
    if (r == FVM_OK) {
        r = w->callback(context, &w->input, &result, w->userdata);
        if (r == FVM_OK) r = copy_value(&w->output, &result);
    }
    w->result = r;
    if (context) fvm_context_destroy(context);
    if (runtime) fvm_runtime_destroy(runtime);
}
FVM_Result fvm_worker_run(const FVM_Config *config, FVM_WorkerFunction callback,
                          const FVM_Value *input, void *userdata, FVM_Worker **out) {
    FVM_Worker *w;
    FVM_Result r;
    if (!config || !callback || !input || !out || config->struct_size != sizeof(FVM_Config))
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    w = (FVM_Worker *)calloc(1, sizeof(*w));
    if (!w) return FVM_OUT_OF_MEMORY;
    w->config = *config;
    w->callback = callback;
    w->userdata = userdata;
    w->result = FVM_ERROR;
    fvm_value_undefined(&w->output);
    r = copy_value(&w->input, input);
    if (r != FVM_OK) { free(w); return r; }
    w->thread = fvm_port_thread_start(worker_entry, w);
    if (!w->thread) { free_value(&w->input); free(w); return FVM_ERROR; }
    *out = w;
    return FVM_OK;
}
FVM_Result fvm_worker_join(FVM_Worker *w, FVM_Value *out) {
    if (!w) return FVM_INVALID_ARGUMENT;
    if (!w->joined) {
        fvm_port_thread_join(w->thread);
        w->thread = 0;
        w->joined = 1;
    }
    if (out) *out = w->output;
    return w->result;
}
void fvm_worker_destroy(FVM_Worker *w) {
    if (!w || !w->joined) return;
    free_value(&w->input);
    free_value(&w->output);
    free(w);
}
