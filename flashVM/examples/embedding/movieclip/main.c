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
/* One stack-owned log outlives every native method registration in the second context. */
struct DepthLog /* Share ordinary C storage, never managed or serialized pointers. */
{ /* Bound observations independently of callback order. */
    unsigned int digits[FVM_TUTORIAL_DEPTH_CHILDREN]; /* Store the low/middle/high identities. */
    unsigned int count; /* Count committed callbacks. */
}; /* End the borrowed host log. */
/* Each registration borrows one identity entry until its context is destroyed. */
struct DepthEntry /* Keep callback identity independent of the native child hash table. */
{ /* A registration owns no C allocation. */
    struct DepthLog *log; /* Borrow the enclosing demonstration's stack-owned log. */
    unsigned int digit; /* Expected ascending-order identity: low=1, middle=2, high=3. */
}; /* End the native registration payload. */
/* Record the actual child order without allocating or advancing another timeline. */
static FVM_Result depth_tick(FVM_Context *context, FVM_Object *receiver, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* Called synchronously on the owner thread. */
{ /* The enclosing tick barrier retains receiver while this userdata is borrowed. */
    struct DepthEntry *entry = (struct DepthEntry *)raw; /* The host keeps this entry alive until cleanup. */
    (void)context; (void)receiver; (void)count; (void)args; /* Only the independent native identity is needed. */
    fvm_value_undefined(out); /* The event route ignores result values. */
    if (entry->log->count >= FVM_TUTORIAL_DEPTH_CHILDREN) /* Detect duplicate delivery before writing the bounded log. */
        return FVM_INVALID_ARGUMENT; /* Preserve completed entries for host inspection. */
    entry->log->digits[entry->log->count++] = entry->digit; /* Commit this receiver's observed position. */
    return FVM_OK; /* Continue the ordered snapshot. */
} /* Finish this native event. */
/* Create children out of depth order in a separate runtime with its own finite work limit. */
static int depth_demo(void) /* No renderer, file, SDL adapter or SWF compiler is needed. */
{ /* Initialize partial owners before the first fallible operation. */
    FVM_Config config; /* Runtime creation copies this second configuration. */
    FVM_Runtime *runtime = 0; /* Outlive the managed context. */
    FVM_Context *context = 0; /* Own the root and all child registrations. */
    FVM_Object *root = 0; /* Pin while retaining borrowed child identities. */
    FVM_Value args[2], out; /* Temporary construction arguments and borrowed child results. */
    struct DepthLog log = {{0,0,0},0}; /* Keep native observation storage alive through context destruction. */
    struct DepthEntry entries[FVM_TUTORIAL_DEPTH_CHILDREN]; /* Borrow one stable entry per registration. */
    const char *names[FVM_TUTORIAL_DEPTH_CHILDREN] = {"high","low","middle"}; /* Deliberately shuffled insertion. */
    const long depths[FVM_TUTORIAL_DEPTH_CHILDREN] = {10,-5,2}; /* Ascending traversal differs from insertion order. */
    const unsigned int digits[FVM_TUTORIAL_DEPTH_CHILDREN] = {3,1,2}; /* Independent expected identities. */
    unsigned int i; /* C89 loop counter declared before statements. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the complete configuration ABI. */
    if (status == FVM_OK) /* Use a separate finite bound for sorting and child delivery. */
    { /* This does not alter the earlier five-unit demonstration. */
        config.instruction_limit = FVM_TUTORIAL_DEPTH_WORK_LIMIT; /* Budget is work units, not milliseconds. */
        status = fvm_runtime_create(&config, &runtime); /* No thread or native device is started. */
    } /* Runtime retains its copied configuration. */
    if (status == FVM_OK) /* Acquire the owner-thread managed heap. */
        status = fvm_context_create(runtime, &context); /* All remaining API calls use this context synchronously. */
    if (status == FVM_OK) /* Install native MovieClip methods before constructing the tree. */
        status = fvm_standard_install(context); /* Preserve errors for cleanup. */
    if (status == FVM_OK) /* Create one frame with no root handler. */
        status = fvm_movieclip_create(context, "ordered", 1, &root); /* The context owns the returned borrowed root. */
    if (status == FVM_OK) /* Retain the complete scene across host boundaries. */
        status = fvm_gc_pin(context, root); /* [GC:] Context destruction releases the pin. */
    for (i = 0; status == FVM_OK && i < FVM_TUTORIAL_DEPTH_CHILDREN; ++i) /* Preserve the intentionally shuffled insertion order. */
    { /* Fully configure one child before constructing the next. */
        entries[i].log = &log; /* Borrow the same stack-owned trace. */
        entries[i].digit = digits[i]; /* Keep identity separate from traversal position. */
        status = fvm_value_string(args, names[i]); /* Prepare a static child name without transferring its C storage. */
        fvm_value_number(args + 1, (double)depths[i]); /* Native public depths may be negative. */
        if (status == FVM_OK) /* Construct through the public optional method bridge. */
            status = fvm_object_call(context, root, "createEmptyMovieClip", 2, args, &out); /* The root retains a successful child. */
        if (status == FVM_OK && (out.type != FVM_VALUE_OBJECT || !out.object)) /* Validate the borrowed method result. */
            status = FVM_INVALID_ARGUMENT; /* Avoid registering on a missing receiver. */
        if (status == FVM_OK) /* No collection happens between construction and registration. */
            status = fvm_object_define_method_rooted(context, out.object, "onEnterFrame", depth_tick, entries + i, 0); /* Stack userdata remains valid until cleanup. */
    } /* Parent graph owns every successfully constructed child. */
    if (status == FVM_OK) /* Snapshot children after the parent tick and visit ascending depths. */
        status = fvm_movieclip_advance(context, root, 1); /* Expected identities are 1,2,3 rather than 3,1,2. */
    if (status == FVM_OK && (log.count != FVM_TUTORIAL_DEPTH_CHILDREN || log.digits[0] != 1 || log.digits[1] != 2 || log.digits[2] != 3)) /* Verify observed identities, not table iteration. */
        status = FVM_INVALID_ARGUMENT; /* Preserve mismatches through ordinary cleanup. */
    if (status == FVM_OK) /* Report only verified observations. */
        puts("depth order: low middle high"); /* Expected third output line of the complete tutorial. */
    else /* A failed tick can already have committed native log entries. */
        fprintf(stderr, "MovieClip depth tutorial failed: %d\n", (int)status); /* Do not replay the event automatically. */
    if (context) /* Release registrations before the borrowed entries/log go out of scope. */
        fvm_context_destroy(context); /* [GC:] No managed pointer is used after return. */
    if (runtime) /* Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* No thread/device cleanup remains. */
    return status == FVM_OK; /* Preserve failure for the caller's exit code. */
} /* End the independent depth-order demonstration. */
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
    if (status == FVM_OK && !depth_demo()) /* Run the independent order example after releasing the first runtime. */
        status = FVM_INVALID_ARGUMENT; /* Propagate the second demonstration's failure. */
    return status == FVM_OK ? 0 : 1; /* Let CTest or a shell observe the result. */
} /* End the standalone host. */
