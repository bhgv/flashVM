/* Execute a captured AVM1 value in an independent native worker. */
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "flashvm/worker.h"
#include <stdio.h>


/* Build a closure, start one worker and release the complete sender heap before
 * joining. All handles stay local; on failure release every acquired resource.
 * Return zero only when the detached closure returns its captured number. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0; /* Sender runtime; never accessed by the worker. */
    FVM_Context *context = 0; /* Sender context, destroyed immediately after launch. */
    FVM_Object *environment = 0; /* Pinned until context destruction. */
    FVM_Object *function = 0; /* Only borrowed across the synchronous launch call. */
    FVM_Worker *worker = 0; /* Owned handle, joined before destruction. */
    FVM_Value value;
    int status = 1; /* Failure unless the independent result is exactly 42. */
    /* ActionPush string "answer"; GetVariable; Return; End. Push payload is eight bytes. */
    const unsigned char code[] = {
        0x96, 8, 0, 0, 'a','n','s','w','e','r',0, 0x1c, 0x3e, 0
    };
    if (fvm_config_init(&config) != FVM_OK ||
        fvm_runtime_create(&config, &runtime) != FVM_OK ||
        fvm_context_create(runtime, &context) != FVM_OK ||
        fvm_standard_install(context) != FVM_OK ||
        fvm_object_create(context, &environment) != FVM_OK ||
        fvm_gc_pin(context, environment) != FVM_OK)
    {
        goto cleanup;
    }
    /* The lexical capture is ordinary data, with no live MovieClip/host pointer. */
    fvm_value_number(&value, 42);
    if (fvm_object_set(context, environment, "answer", &value) != FVM_OK ||
        fvm_function_create_avm1(context, code, sizeof(code), 0, 0,
                                 environment, &function) != FVM_OK ||
        fvm_worker_run_function(context, &config, function, &worker) != FVM_OK)
    {
        goto cleanup;
    }
    /* Launch has copied the graph. The worker cannot call back into this heap. */
    fvm_context_destroy(context);
    context = 0;
    fvm_runtime_destroy(runtime);
    runtime = 0;
    /* Join is deliberately blocking here; an interactive owner should poll
     * fvm_worker_result and join after completion instead of blocking input. */
    if (fvm_worker_join(worker, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 42)
    {
        puts("Detached captured answer: 42");
        status = 0;
    }
cleanup:
    /* A launched handle must always be joined, including error paths. */
    if (worker)
    {
        fvm_worker_join(worker, 0);
        fvm_worker_destroy(worker);
    }
    if (context)
    {
        fvm_context_destroy(context);
    }
    if (runtime)
    {
        fvm_runtime_destroy(runtime);
    }
    return status;
}
