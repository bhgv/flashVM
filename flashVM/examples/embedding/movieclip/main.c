/* C89 tutorial: bounded stopped MovieClip ticks and committed callback effects; see README.md. */
#include "flashvm/movieclip.h" /* Create a timeline and drive its owner-thread ticks explicitly. */
#include "flashvm/standard.h" /* Install the MovieClip class and runtime globals. */
#include "flashvm/gc.h" /* Pin the host-owned root between calls; context destruction releases it. */
#include "tutorial_limits.h" /* Name this tutorial's deliberate work limit and batch size. */
#include <stdio.h> /* Report ordinary errors and deterministic callback counts. */
/* Borrow the counter until context destruction; this callback owns no managed capture. */
static FVM_Result tick(FVM_Context *context, FVM_Object *receiver, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* See README.md for callback ownership. */
{ /* Execute synchronously inside the native tick barrier. */
    unsigned int *calls = (unsigned int *)raw; /* The host keeps this counter alive throughout cleanup. */
    (void)context; (void)receiver; (void)count; (void)args; /* No script arguments or VM operations are needed. */
    ++*calls; /* Preserve this effect even if a later tick exceeds its budget. */
    fvm_value_undefined(out); /* Optional event delivery ignores the result value. */
    return FVM_OK; /* Permit the bounded traversal to continue. */
} /* Do not destroy or access the context from another thread here. */
/* Demonstrate partial completion and the fresh budget of a later outer host call. */
int main(void) /* Build with the independent FlashVM component's examples option. */
{ /* Set partial owners before any fallible acquisition. */
    FVM_Config config; /* Runtime creation copies the execution limit. */
    FVM_Runtime *runtime = 0; /* Outlive the context. */
    FVM_Context *context = 0; /* Own the stopped clip and callback registration. */
    FVM_Object *clip = 0; /* Pin before a host collector can run. */
    FVM_Value ignored; /* Receive the result of stop(). */
    unsigned int calls = 0; /* Borrowed callback userdata remains valid until destruction. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the complete configuration ABI. */
    if (status == FVM_OK) /* Use a small limit to make partial completion deterministic. */
    { /* Change only the ordinary instruction/work bound. */
        config.instruction_limit = FVM_TUTORIAL_TICK_WORK_LIMIT; /* Native traversal and AVM1 callbacks share this budget. */
        status = fvm_runtime_create(&config, &runtime); /* No worker or device is created. */
    } /* A runtime owns a copy of config. */
    if (status == FVM_OK) /* Acquire the managed heap on this thread. */
        status = fvm_context_create(runtime, &context); /* Use this thread for every following VM call. */
    if (status == FVM_OK) /* Install standard MovieClip methods before construction. */
        status = fvm_standard_install(context); /* Host mode needs no SWF bytes. */
    if (status == FVM_OK) /* A single-frame clip is enough to demonstrate tick handlers. */
        status = fvm_movieclip_create(context, "root", 1, &clip); /* The returned pointer is borrowed from the context. */
    if (status == FVM_OK) /* Retain the graph across host call boundaries. */
        status = fvm_gc_pin(context, clip); /* [GC:] Context destruction releases this root. */
    if (status == FVM_OK) /* Pausing a timeline does not disable its onEnterFrame handler. */
        status = fvm_object_call(context, clip, "stop", 0, 0, &ignored); /* Stop frame advancement, not host ticks. */
    if (status == FVM_OK) /* Register the optional native handler without a managed userdata object. */
        status = fvm_object_define_method_rooted(context, clip, "onEnterFrame", tick, &calls, 0); /* The clip retains its registration. */
    if (status == FVM_OK) /* Request three ticks using a single shared five-unit budget. */
    { /* Accept only the intentional limit error after two completed callbacks. */
        status = fvm_movieclip_advance(context, clip, FVM_TUTORIAL_TICK_BATCH); /* The third node entry commits before its handler budget fails. */
        if (status == FVM_ERROR && calls == 2) /* Partial delivery is documented, not a rollback transaction. */
        { /* A new outer call starts a fresh work budget. */
            puts("bounded batch: 2 callbacks retained"); /* Deterministic first output line. */
            status = fvm_movieclip_advance(context, clip, 1); /* Deliver one more handler on the stopped clip. */
        } /* Do not automatically replay the failed three-tick batch. */
        else /* Any unexpected status or count is a tutorial failure. */
            status = FVM_INVALID_ARGUMENT; /* Report the mismatch through normal cleanup. */
    } /* The callback remains registered while its userdata is alive. */
    if (status == FVM_OK && calls != 3) /* Verify that the second call delivered exactly once. */
        status = FVM_INVALID_ARGUMENT; /* Preserve a failing result for the exit code. */
    if (status == FVM_OK) /* Report the complete demonstration. */
        printf("after next call: %u callbacks\n", calls); /* Expected count is three. */
    else /* Report errors before releasing owners. */
        fprintf(stderr, "MovieClip tutorial failed: %d\n", (int)status); /* Keep diagnostics independent of managed storage. */
    if (context) /* Release the pinned graph and registration before counter storage expires. */
        fvm_context_destroy(context); /* [GC:] No callback occurs after this returns. */
    if (runtime) /* Release the copied configuration last. */
        fvm_runtime_destroy(runtime); /* No external platform resource is owned. */
    return status == FVM_OK ? 0 : 1; /* Let CTest or a shell observe the result. */
} /* End the standalone host. */
