/* FlashVM C89 tutorial: read the shared context clock through the AS1/AS2 global. */
#include "flashvm/standard.h" /* Runtime types and standard installation; see README.md API table. */
#include <stdio.h> /* Print a platform-dependent elapsed millisecond sample. */
/* Create an owning-thread context, read its clock and release both owners on every path. */
int main(void) /* No SWF, network, display, worker or sleeping is required. */
{ /* Keep partial acquisition safe for the common cleanup path. */
    FVM_Config config; /* Configuration copied by runtime creation. */
    FVM_Runtime *runtime = 0; /* Outer owner outlives the context. */
    FVM_Context *context = 0; /* Owns standard bindings and the timer origin. */
    FVM_Value elapsed; /* Receive a numeric value without a managed result buffer. */
    FVM_Result status = fvm_config_init(&config); /* Initialize ABI defaults before creation. */
    if (status == FVM_OK) /* Acquire the outer owner after valid configuration. */
        status = fvm_runtime_create(&config, &runtime); /* Creates no asynchronous work here. */
    if (status == FVM_OK) /* Establish this context's independent monotonic origin. */
        status = fvm_context_create(runtime, &context); /* A failed clock can recover at a later read. */
    if (status == FVM_OK) /* Publish getTimer and its standard-library dependencies. */
        status = fvm_standard_install(context); /* Preserve pre-existing conflicting global names. */
    if (status == FVM_OK) /* Read synchronously without waiting or scheduling a callback. */
        status = fvm_call(context, "getTimer", 0, 0, &elapsed); /* See README.md and the GET_TIMER guide. */
    if (status == FVM_OK && elapsed.type != FVM_VALUE_NUMBER) /* Validate the documented result before use. */
        status = FVM_ERROR; /* Route unexpected results through ordinary cleanup. */
    if (status == FVM_OK) /* The actual value depends on platform timing and setup duration. */
        printf("context elapsed milliseconds: %.0f\n", elapsed.number); /* Expect a nonnegative integer below 2^32. */
    else /* Clock, memory or installation failure is reported to the caller. */
        fprintf(stderr, "getTimer example failed: %d\n", (int)status); /* Do not substitute a fabricated time. */
    if (context) /* End all context use before releasing the runtime. */
        fvm_context_destroy(context); /* [GC:] Release standard objects and their heap. */
    if (runtime) /* Release the outer owner last, even after a partial failure. */
        fvm_runtime_destroy(runtime); /* No result owns memory after context destruction. */
    return status == FVM_OK ? 0 : 1; /* Expose success or an unexpected error to shell/CTest. */
} /* The example leaves no running timer or callback behind. */
