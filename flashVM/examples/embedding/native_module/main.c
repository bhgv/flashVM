/* Run a separately compiled C module through real FlashVM calls, GC and optional SWF actions. */
#include "module.h"
#include "flashvm/flashvm.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <stdlib.h>

#define DEMO_SWF_LIMIT 1048576L /* Only bounded explicit local example inputs are read. */


/* Read a caller-selected linked SWF. Own the returned bytes until after context
 * teardown; on failure returnNULL and leave length0. Never generate bytecode. */
static unsigned char *read_swf(const char *path, size_t *length)
{
    FILE *file;
    long size;
    unsigned char *bytes;
    *length = 0;
    file = fopen(path, "rb");
    if (!file)
    {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) || (size = ftell(file)) <= 0 || size > DEMO_SWF_LIMIT || fseek(file, 0, SEEK_SET))
    {
        fclose(file);
        return NULL;
    }
    bytes = (unsigned char *)malloc((size_t)size);
    if (!bytes || fread(bytes, 1, (size_t)size, file) != (size_t)size)
    {
        free(bytes);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *length = (size_t)size;
    return bytes;
}


/* Preserve evaluation in release builds. Labels describe observable contracts;
 * cleanup remains in main so no native callback/state outlives its context. */
static int require(int condition, const char *label)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL native module: %s\n", label);
    }
    return condition;
}


/* With no argument test C namespace calls. An optional real linker-produced SWF
 * also executes AS member writes to the native accessor, without a frame tick. */
int main(int argc, char **argv)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    FVM_Context *foreign = NULL;
    FVM_Object *capture = NULL;
    FVM_Object *module = NULL;
    FVM_Object *unchanged;
    FVM_Value value;
    FVM_Value arguments[2];
    FVM_Value result;
    DemoNativeState state = {0, 0, NULL};
    DemoNativeState rejected = {0, 0, NULL};
    unsigned char *swf = NULL;
    size_t swf_size = 0;
    unsigned long reclaimed;
    int status = 1;
    int capture_pinned = 0;
    int module_pinned = 0;
    if (argc > 2)
    {
        fprintf(stderr, "usage: native-module-example [linked-example.swf]\n");
        return 2;
    }
    if (!require(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
                     fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK,
                 "initialize actual runtime/context"))
    {
        goto cleanup;
    }
    if (argc == 2)
    {
        swf = read_swf(argv[1], &swf_size);
        if (!require(swf && fvm_load_memory(context, swf, swf_size) == FVM_OK, "load explicit linked SWF"))
        {
            goto cleanup;
        }
    }
    if (!require(fvm_object_create(context, &capture) == FVM_OK && fvm_gc_pin(context, capture) == FVM_OK,
                 "root managed capture before allocation"))
    {
        goto cleanup;
    }
    capture_pinned = 1;
    fvm_value_number(&value, 42);
    if (!require(fvm_object_set(context, capture, "value", &value) == FVM_OK &&
                     demo_native_module_create(context, &state, capture, &module) == FVM_OK &&
                     fvm_gc_pin(context, module) == FVM_OK,
                 "build a complete rooted namespace"))
    {
        goto cleanup;
    }
    module_pinned = 1;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = module;
    if (!require(fvm_set_global(context, "NativeDemo", &value) == FVM_OK, "publish with the existing embedding API"))
    {
        goto cleanup;
    }
    fvm_gc_unpin(context, module);
    module_pinned = 0;
    fvm_gc_unpin(context, capture);
    capture_pinned = 0;
    if (!require(fvm_gc_collect(context, &reclaimed) == FVM_OK &&
                     fvm_object_call(context, module, "seed", 0, NULL, &result) == FVM_OK &&
                     result.type == FVM_VALUE_NUMBER && result.number == 42,
                 "method capture survives actual collection"))
    {
        goto cleanup;
    }
    fvm_value_number(&arguments[0], 19);
    fvm_value_number(&arguments[1], 23);
    if (!require(fvm_object_call(context, module, "add", 2, arguments, &result) == FVM_OK &&
                     result.type == FVM_VALUE_NUMBER && result.number == 42 && state.calls == 1,
                 "invoke a real C method and return its scalar result"))
    {
        goto cleanup;
    }
    if (!require(fvm_object_call(context, module, "add", 1, arguments, &result) == FVM_INVALID_ARGUMENT &&
                     state.calls == 1,
                 "wrong arity has no module state side effect"))
    {
        goto cleanup;
    }
    if (!require(fvm_context_create(runtime, &foreign) == FVM_OK, "create independent owner"))
    {
        goto cleanup;
    }
    unchanged = module;
    if (!require(demo_native_module_create(foreign, &rejected, capture, &unchanged) == FVM_INVALID_ARGUMENT &&
                     unchanged == module && rejected.seed_owner == NULL,
                 "foreign capture is refused before publication"))
    {
        goto cleanup;
    }
    if (argc == 2 && !require(fvm_swf_execute_actions(context) == FVM_OK && state.total == 42 && state.calls == 3,
                              "execute real compiled AS writes to the native module without frame advancement"))
    {
        goto cleanup;
    }
    puts("PASS native C module: namespace, scalar method, rooted capture, refusal and optional real AS/SWF setter");
    status = 0;
cleanup:
    if (module_pinned)
    {
        fvm_gc_unpin(context, module);
    }
    if (capture_pinned)
    {
        fvm_gc_unpin(context, capture);
    }
    /* Both instances of host state remain alive through every callback retirement. */
    fvm_context_destroy(foreign);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    free(swf);
    return status;
}
