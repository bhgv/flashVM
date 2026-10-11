/* C89: transfer a shared mutex to a worker through a channel of VM values. */
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include "flashvm/worker.h"
#include <assert.h>
#include <stdio.h>
static FVM_Result worker(FVM_Context *c, const FVM_Value *in, FVM_Value *out, void *user)
{
    FVM_Value lock, result;
    FVM_Result r;
    (void)in;
    r = fvm_channel_receive_value(c, (FVM_Channel *)user, &lock, 1);
    if (r != FVM_OK)
        return r;
    r = fvm_object_call(c, lock.object, "set", 0, 0, &result);
    if (r != FVM_OK)
        return r;
    /* Shared protected application work belongs here. */
    r = fvm_object_call(c, lock.object, "reset", 0, 0, &result);
    fvm_value_boolean(out, r == FVM_OK);
    return r;
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Channel *pipe;
    FVM_Worker *task;
    FVM_Value mutex, lock, input, result;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_extensions_install(c) == FVM_OK);
    assert(fvm_get_global(c, "mutex", &mutex) == FVM_OK);
    assert(fvm_object_call(c, mutex.object, "new", 0, 0, &lock) == FVM_OK);
    assert(fvm_channel_create(&pipe) == FVM_OK);
    assert(fvm_channel_send_value(c, pipe, &lock, 0) == FVM_OK);
    fvm_value_undefined(&input);
    assert(fvm_worker_run(&cfg, worker, &input, pipe, &task) == FVM_OK);
    assert(fvm_worker_join(task, &result) == FVM_OK && result.boolean);
    printf("Shared mutex acquired and released by the worker.\n");
    fvm_worker_destroy(task);
    fvm_channel_destroy(pipe);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
