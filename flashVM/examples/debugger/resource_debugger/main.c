/* Debugger resource tutorial: independent observers and queued owner resume; see README.md. */
#include "flashvm/rdebug.h"   /* Resource adapter and copied client-side stop decoder. */
#include "flashvm/function.h" /* Construct/invoke the demonstrated AVM1 function. */
#include "flashvm/gc.h"       /* Explicitly retain the function across host setup and execution. */
#include <stdio.h>  /* Optional presentation; commands use the C API, not console input. */
#include <string.h> /* Initialize native state without copying VM ownership. */
/* [GC:] Host storage owns native queues and borrows no managed object from an observer. */
typedef struct Host /* Describe caller-owned native state for the resource example. */
{ /* Two authenticated local sessions model independent debugger clients. */
    FVM_RDispatch *dispatch; /* Owned until callbacks and producers have stopped. */
    FVM_RHandle resource;    /* Owner-registered debugger resource generation. */
    FVM_RWatch watches[2]; /* Separate event queues; readers cannot consume each other's events. */
} Host;                    /* Native host data outlives the resource adapter. */
/* A real app blocks on its transport/event loop here; this deterministic tutorial queues input. */
static FVM_Result service(FVM_RDebug *debug, void *raw) /* Called by the stopped VM owner only. */
{ /* No reentry, GC, context destruction or VM worker access is allowed here. */
    Host *host = (Host *)raw;    /* Borrow the caller-owned host descriptor. */
    FVM_REvent event;            /* Native copy of one observer's event. */
    FVM_RDebugSnapshot snapshot; /* Decode portable bytes into independent metadata. */
    FVM_RRequest request;        /* Dispatcher copies this request and payload before returning. */
    unsigned char /* Keep this native declaration or continuation within its documented owner scope. */
        expected[FVM_SRECORD_INTEGER_MAX]; /* Canonical S integer for the expected stop. */
    FVM_SInteger integer = {0, 0, 0};      /* No native-width byte copy appears on wire. */
    size_t expected_size;   /* Actual encoded integer size, excluding unused buffer capacity. */
    unsigned long stop = 0; /* Both observers must agree about this stop incarnation. */
    size_t i;               /* Visit both independently subscribed sessions. */
    (void)debug; /* This host controls through resource commands rather than adapter internals. */
    for (i = 0; i < 2; ++i) /* Keep this native declaration or continuation within its documented owner scope. */
    { /* Taking session one leaves session two's event intact. */
        if (fvm_rdispatch_event_take(host->dispatch, (unsigned long)i + 1, host->watches[i], /* Check fvm_rdispatch_event_take before using its output. */
                                     &event) != FVM_R_OK) /* Keep this native declaration or continuation within its documented owner scope. */
            return FVM_ERROR; /* Require one copied stop. */
        if (event.kind != FVM_RDEBUG_STOP_EVENT || /* Validate the owner-side condition before continuing. */
            fvm_rdebug_snapshot_read(event.data, event.size, &snapshot) != FVM_R_OK) /* Call fvm_rdebug_snapshot_read on the owner; no worker accesses this context. */
            return FVM_ERROR; /* Strict payload validation. */
        if (i && snapshot.stop != stop) /* Validate the owner-side condition before continuing. */
            return FVM_ERROR; /* Verify the same owner stop. */
        stop = snapshot.stop; /* Preserve the integer, not any VM frame pointer. */
        printf("observer=%lu stop=%lu frames=%lu\n", (unsigned long)i + 1, stop, /* Continue the explicitly borrowed arguments of this native call. */
               snapshot.frames); /* Show copied structural information. */
    } /* Both readers received STOP before the VM continues. */
    memset(&request, 0, sizeof(request)); /* Initialize all native descriptor fields. */
    request.session = 1; /* Keep this native declaration or continuation within its documented owner scope. */
    request.id = 1; /* Keep this native declaration or continuation within its documented owner scope. */
    request.target = host->resource; /* Trusted local identity, not a wire claim. */
    request.operation = /* Keep this native declaration or continuation within its documented owner scope. */
        FVM_RDEBUG_CLAIM; /* Session one asks to become the sole mutating controller. */
    if (fvm_rdispatch_post(host->dispatch, &request) != FVM_R_OK) /* Check fvm_rdispatch_post before using its output. */
        return FVM_ERROR; /* Actual execution occurs in the callback's next owner pump. */
    integer.low = stop;   /* Stop IDs use the nonnegative Uint32 integer domain. */
    if (fvm_sinteger_write(&integer, expected, sizeof(expected), &expected_size) != FVM_S_OK) /* Check fvm_sinteger_write before using its output. */
        return FVM_ERROR; /* Canonical compact big-endian integer. */
    request.id = 2; /* Keep this native declaration or continuation within its documented owner scope. */
    request.operation = FVM_RDEBUG_RESUME; /* Resume follows claim in the same owner FIFO. */
    request.data = expected; /* Keep this native declaration or continuation within its documented owner scope. */
    request.size = expected_size; /* Input is borrowed only through post. */
    if (fvm_rdispatch_post(host->dispatch, &request) != FVM_R_OK) /* Check fvm_rdispatch_post before using its output. */
        return FVM_ERROR; /* No direct call into the interpreter is made. */
    return FVM_OK; /* The adapter pumps both commands and returns from the breakpoint callback. */
} /* Production services must also handle wakeups, disconnect and shutdown. */
/* Run one real AVM1 function and release its debugger hook before native adapter storage. */
int main(void) /* No console arguments, SDK window, filesystem or network endpoint is required. */
{              /* Initialize every owned handle so partial setup can use common cleanup. */
    const unsigned char body[] = {0x96, 5, 0, 7,    42, /* Continue the explicitly borrowed arguments of this native call. */
                                  0,    0, 0, 0x3e, 0}; /* AVM1 Push integer 42; Return; End. */
    FVM_Config config; /* Keep this native declaration or continuation within its documented owner scope. */
    FVM_Runtime *runtime = 0; /* Keep this native declaration or continuation within its documented owner scope. */
    FVM_Context *context = 0; /* Own runtime/context until teardown. */
    FVM_Object *function = 0; /* Keep this native declaration or continuation within its documented owner scope. */
    FVM_Value result; /* Function is pinned explicitly before setup allocations. */
    FVM_RDebug *debug = 0; /* Keep this native declaration or continuation within its documented owner scope. */
    FVM_RReply reply; /* Keep this native declaration or continuation within its documented owner scope. */
    FVM_REvent event; /* Native debugger and copied outputs. */
    Host host; /* Keep this native declaration or continuation within its documented owner scope. */
    size_t i; /* Keep this native declaration or continuation within its documented owner scope. */
    int pinned = 0, status = 1; /* Preserve partial-setup ownership flags. */
    memset(&host, 0, /* Continue the explicitly borrowed arguments of this native call. */
           sizeof(host)); /* Resource and watch outputs are published only after success. */
    if (fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK) /* Check fvm_config_init before using its output. */
        goto cleanup; /* Check runtime setup. */
    if (fvm_context_create(runtime, &context) != FVM_OK) /* Check fvm_context_create before using its output. */
        goto cleanup; /* This context stays on the main owner thread. */
    if (fvm_function_create_avm1(context, body, sizeof(body), 0, 0, 0, &function) != FVM_OK) /* Check fvm_function_create_avm1 before using its output. */
        goto cleanup; /* Copy bytecode into a managed function. */
    if (fvm_gc_pin(context, function) != FVM_OK) /* Check fvm_gc_pin before using its output. */
        goto cleanup; /* [GC:] Retain the function across subsequent native calls. */
    pinned = 1;       /* Exactly one matching unpin is now required. */
    if (fvm_rdispatch_create(0, &host.dispatch) != FVM_R_OK) /* Check fvm_rdispatch_create before using its output. */
        goto cleanup; /* Single-owner tutorial; real shared hosts supply locks. */
    if (fvm_rdebug_create(context, host.dispatch, "/debug/ctl", service, &host, &debug, /* Check fvm_rdebug_create before using its output. */
                          &host.resource) != FVM_R_OK) /* Keep this native declaration or continuation within its documented owner scope. */
        goto cleanup; /* Register without replacing a debugger hook. */
    for (i = 0; i < 2; ++i) /* Keep this native declaration or continuation within its documented owner scope. */
    { /* The observer session receives no mutating CONTROL permission. */
        unsigned int rights = /* Keep this native declaration or continuation within its documented owner scope. */
            FVM_R_READ | FVM_R_SUBSCRIBE; /* Both can inspect copied stop events. */
        if (!i) /* Validate the owner-side condition before continuing. */
            rights |= FVM_R_CONTROL; /* Only session one can claim control and resume. */
        if (fvm_rdispatch_grant(host.dispatch, host.resource, (unsigned long)i + 1, rights) != /* Check fvm_rdispatch_grant before using its output. */
            FVM_R_OK) /* Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup; /* Owner chooses authority. */
        if (fvm_rdispatch_subscribe(host.dispatch, (unsigned long)i + 1, host.resource, /* Check fvm_rdispatch_subscribe before using its output. */
                                    host.watches + i) != FVM_R_OK) /* Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup; /* Each queue starts empty. */
    } /* No authentication or remote routing is supplied by numeric identities. */
    if (fvm_debug_set_breakpoint_handler(context, fvm_rdebug_breakpoint, debug) != FVM_OK) /* Check fvm_debug_set_breakpoint_handler before using its output. */
        goto cleanup; /* Explicitly install the borrowed adapter callback. */
    if (fvm_debug_set_breakpoint(context, "<function>", 0, 1) != FVM_OK) /* Check fvm_debug_set_breakpoint before using its output. */
        goto cleanup; /* Stop before the real function's first action. */
    if (fvm_function_invoke(context, function, 0, 0, 0, &result) != FVM_OK || result.number != 42) /* Check fvm_function_invoke before using its output. */
        goto cleanup; /* Owner services the stop, resumes, then returns 42. */
    for (i = 1; i <= 2; ++i) /* Keep this native declaration or continuation within its documented owner scope. */
    { /* Acknowledge both command identities after copying their outcomes. */
        if (fvm_rdispatch_take(host.dispatch, 1, (unsigned long)i, &reply) != FVM_R_OK || /* Check fvm_rdispatch_take before using its output. */
            reply.result != FVM_R_OK) /* Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup; /* Free bounded command capacity. */
    } /* Resume acceptance differs from the completed invocation verified above. */
    for (i = 0; i < 2; ++i) /* Keep this native declaration or continuation within its documented owner scope. */
    { /* Both observers also receive the callback-release event. */
        if (fvm_rdispatch_event_take(host.dispatch, (unsigned long)i + 1, host.watches[i], /* Check fvm_rdispatch_event_take before using its output. */
                                     &event) != FVM_R_OK || /* Keep this native declaration or continuation within its documented owner scope. */
            event.kind != FVM_RDEBUG_RESUME_EVENT) /* Keep this native declaration or continuation within its documented owner scope. */
            goto cleanup; /* Independent reads. */
    } /* The function result separately confirms real execution progress. */
    puts( /* Keep this native declaration or continuation within its documented owner scope. */
        "result=42; both observers received stop and resume"); /* Expected final tutorial output. */
    status = 0; /* All demonstrated outcomes were confirmed. */
cleanup:        /* Stop callback use before releasing its borrowed native storage. */
    if (context) /* Validate the owner-side condition before continuing. */
        fvm_debug_set_breakpoint_handler(context, 0, 0); /* Detach even after partial setup. */
    if (host.dispatch) /* Validate the owner-side condition before continuing. */
    { /* This single-threaded host has no concurrent producers to join. */
        if (fvm_rdispatch_disconnect(host.dispatch, 1) != FVM_R_OK) /* Check fvm_rdispatch_disconnect before using its output. */
            status = 1; /* Discard any queued commands/replies from failed setup. */
        if (fvm_rdispatch_disconnect(host.dispatch, 2) != FVM_R_OK) /* Check fvm_rdispatch_disconnect before using its output. */
            status = 1; /* Close the second observer independently. */
    } /* A failed command is never treated as a rollback of VM side effects. */
    if (debug && fvm_rdebug_destroy(&debug) != FVM_R_OK) /* Check fvm_rdebug_destroy before using its output. */
        status = 1; /* Unregister before freeing adapter userdata. */
    if (host.dispatch && fvm_rdispatch_destroy(&host.dispatch) != FVM_R_OK) /* Check fvm_rdispatch_destroy before using its output. */
        status = 1; /* All request capacity has been released. */
    if (pinned) /* Validate the owner-side condition before continuing. */
        fvm_gc_unpin(context, /* Call fvm_gc_unpin on the owner; no worker accesses this context. */
                     function); /* [GC:] Match the sole host pin before destroying the context. */
    if (context) /* Validate the owner-side condition before continuing. */
        fvm_context_destroy(context); /* Release managed functions after callback detachment. */
    if (runtime) /* Validate the owner-side condition before continuing. */
        fvm_runtime_destroy(runtime); /* Release the enclosing runtime last. */
    return status; /* Library control remains independent from console presentation. */
} /* End the real debugger-resource tutorial. */
