/* Actual AVM1 closure execution across independently owned worker heaps. */
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/capability.h"
#include "flashvm/standard.h"
#include "flashvm/worker.h"
#include <stdio.h>
#include <stdlib.h>


/* Fail the executable with a useful boundary name; condition is evaluated once.
 * label is borrowed static text. No cleanup is needed after process failure. */
static void require(int condition, const char *label)
{
    if (!condition)
    {
        fprintf(stderr, "worker function: %s\n", label);
        exit(EXIT_FAILURE);
    }
}


/* Return undefined from a deliberately unsupported native entry. No captures,
 * arguments or receiver are used, and no state is retained or shared. */
static FVM_Result native_entry(FVM_Context *context, FVM_Object *receiver,
                               unsigned int count, const FVM_Value *arguments,
                               FVM_Value *output, void *unused)
{
    (void)context;
    (void)receiver;
    (void)count;
    (void)arguments;
    (void)unused;
    fvm_value_undefined(output);
    return FVM_OK;
}


/* Launch four copies of a mutating closure, destroy the source before joins,
 * and check isolated captures, rejection and primitive result ownership. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *environment = 0;
    FVM_Object *function = 0;
    FVM_Object *native = 0;
    FVM_Object *object_result = 0;
    FVM_Worker *object_worker = 0;
    FVM_Worker *limited_worker = 0;
    FVM_Config limited;
    FVM_Worker *workers[4]; /* Independently owned thread handles. */
    FVM_Worker *rejected = 0;
    FVM_Value value;
    unsigned int i;
    /* counter = counter + 1; return counter; embedded NULs survive transport. */
    const unsigned char code[] = {
        0x96, 9, 0, 0, 'c','o','u','n','t','e','r',0,
        0x96, 9, 0, 0, 'c','o','u','n','t','e','r',0, 0x1c,
        0x96, 5, 0, 7, 1,0,0,0, 0x0a, 0x1d,
        0x96, 9, 0, 0, 'c','o','u','n','t','e','r',0, 0x1c, 0x3e, 0
    };
    const unsigned char self_code[] = {
        0x96, 6, 0, 0, 's','e','l','f',0, 0x1c, 0x3e, 0
    };
    require(fvm_config_init(&config) == FVM_OK, "configuration");
    require(fvm_runtime_create(&config, &runtime) == FVM_OK, "runtime");
    require(fvm_context_create(runtime, &context) == FVM_OK, "context");
    require(fvm_standard_install(context) == FVM_OK, "standard library");
    require(fvm_object_create(context, &environment) == FVM_OK, "environment");
    require(fvm_gc_pin(context, environment) == FVM_OK, "pin environment");
    fvm_value_number(&value, 41);
    require(fvm_object_set(context, environment, "counter", &value) == FVM_OK,
            "initial captured value");
    require(fvm_function_create_avm1(context, code, sizeof(code), 0, 0,
                                    environment, &function) == FVM_OK, "closure");
    require(fvm_gc_pin(context, function) == FVM_OK, "pin closure");
    require(fvm_function_create_native(context, native_entry, 0, 0, &native) == FVM_OK,
            "native refusal fixture");
    require(fvm_worker_run_function(context, &config, native, &rejected) == FVM_UNSUPPORTED &&
            !rejected, "native entry cannot silently run on owner");
    require(fvm_worker_run_function(context, &config, 0, &rejected) == FVM_INVALID_ARGUMENT &&
            !rejected, "null entry");
#ifdef OS_PORTS
    {
        FVM_ResourceBroker *broker = 0;
        require(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK,
                "live-transfer broker");
        require(fvm_context_set_broker(context, broker) == FVM_OK, "bind broker");
        require(fvm_context_function_transfer(context, 1) == FVM_OK, "enable live transfer");
        require(fvm_worker_run_function(context, &config, function, &rejected) == FVM_UNSUPPORTED &&
                !rejected, "live proxy must not masquerade as detached execution");
        require(fvm_context_function_transfer(context, 0) == FVM_OK, "restore snapshot transfer");
        require(fvm_context_set_broker(context, 0) == FVM_OK, "detach broker");
        fvm_broker_stop(broker);
        fvm_broker_release(broker);
    }
#endif
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = environment;
    require(fvm_object_set(context, environment, "self", &value) == FVM_OK,
            "cyclic environment");
    require(fvm_function_create_avm1(context, self_code, sizeof(self_code), 0, 0,
                                    environment, &object_result) == FVM_OK,
            "object return fixture");
    require(fvm_gc_pin(context, object_result) == FVM_OK, "pin object fixture");
    require(fvm_worker_run_function(context, &config, object_result, &object_worker) == FVM_OK,
            "object return launch");
    limited = config;
    limited.instruction_limit = 1;
    require(fvm_worker_run_function(context, &limited, function, &limited_worker) == FVM_OK,
            "bounded execution launch");
    for (i = 0; i < 4; ++i)
    {
        {
            FVM_Result result = fvm_worker_run_function(context, &config, function, &workers[i]);
            if (result != FVM_OK)
            {
                fprintf(stderr, "launch result %d\n", (int)result);
            }
            require(result == FVM_OK, "launch copied closure");
        }
    }
    require(fvm_object_get(context, environment, "counter", &value) == FVM_OK &&
            value.type == FVM_VALUE_NUMBER && value.number == 41,
            "workers cannot mutate original capture");
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    for (i = 0; i < 4; ++i)
    {
        require(fvm_worker_join(workers[i], &value) == FVM_OK &&
                value.type == FVM_VALUE_NUMBER && value.number == 42,
                "each isolated copy returns 42 after source destruction");
        fvm_worker_destroy(workers[i]);
    }
    require(fvm_worker_join(object_worker, &value) == FVM_INVALID_ARGUMENT,
            "managed result cannot escape worker heap");
    fvm_worker_destroy(object_worker);
    require(fvm_worker_join(limited_worker, &value) == FVM_ERROR,
            "worker instruction budget is enforced");
    fvm_worker_destroy(limited_worker);
    puts("PASS detached AVM1 closure workers");
    return 0;
}
