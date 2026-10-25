/* Exactly the adapter's native allocation fails; no VM hook or resource is partially published. */
#include "flashvm/rdebug.h"
#include <stdio.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "adapter allocation line %d\n", __LINE__);                                                 \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)
#include <stdlib.h>

static int fail;


static void *allocate(size_t count, size_t size)
{
    return fail ? 0 : calloc(count, size);
}

#define calloc allocate
#include "../../src/capi/rdebug.c"

#undef calloc


static FVM_Result service(FVM_RDebug *d, void *raw)
{
    (void)d;
    (void)raw;
    return FVM_ERROR;
}


int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_RDispatch *dispatch;
    FVM_RDebug *debug = (FVM_RDebug *)1;
    FVM_RHandle handle = {77, 88};
    size_t count = 99;
    CHECK(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK);
    CHECK(fvm_rdispatch_create(0, &dispatch) == FVM_R_OK);
    fail = 1;
    CHECK(fvm_rdebug_create(context, dispatch, "/debug/ctl", service, 0, &debug, &handle) == FVM_R_NOMEM);
    CHECK(debug == (FVM_RDebug *)1 && handle.id == 77 && handle.generation == 88);
    fail = 0;
    CHECK(fvm_rdebug_create(context, dispatch, "/../bad", service, 0, &debug, &handle) == FVM_R_INVALID);
    CHECK(debug == (FVM_RDebug *)1 && handle.id == 77 && handle.generation == 88);
    CHECK(fvm_rdispatch_resources(dispatch, 1, 0, 0, &count) == FVM_R_OK && count == 0);
    CHECK(fvm_rdispatch_destroy(&dispatch) == FVM_R_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
