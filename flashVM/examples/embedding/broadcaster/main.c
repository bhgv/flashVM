/* Generic events without a window; see doc/AS_BROADCASTER.md for the supported profile. */
#include "flashvm/broadcaster.h" /* Install an optional namespace and initialize an ordinary object. */
#include "flashvm/standard.h" /* Supply native Object and Array before broadcaster initialization. */
#include "flashvm/gc.h" /* [GC:] Keep partially constructed objects alive until cleanup. */
#include "flashvm/transfer.h" /* Snapshot graphs through a context-independent channel. */
#include "flashvm/serialization.h" /* Register native code identity in both contexts. */
#include <stdio.h> /* Print the payload and report the original failure code. */
/* Receive borrowed arguments synchronously; retain no VM pointers after return. */
static FVM_Result receive(FVM_Context *context, FVM_Object *self, unsigned int count, /* Borrow owners. */
                          const FVM_Value *args, FVM_Value *out, void *data) /* Borrow payload storage. */
{ /* Begin the receiver's onValue method. */
    (void)context; /* No recursive VM work is needed in this example. */
    (void)self; /* The dispatcher binds this to the registered listener. */
    (void)data; /* This callback needs no native capture. */
    if (count != 1 || args[0].type != FVM_VALUE_NUMBER) /* Check the application event contract. */
        return FVM_INVALID_ARGUMENT; /* Preserve ordinary error handling in tutorial code. */
    printf("onValue: %.0f\n", args[0].number); /* Expected payload: 42. */
    fvm_value_undefined(out); /* Event delivery ignores a successful handler's result. */
    return FVM_OK; /* Continue to the next listener. */
} /* End borrowed callback lifetime. */
/* Clone the broadcaster into a second heap; native callback code stays in this executable. */
static FVM_Result deliver_copy(FVM_Runtime *runtime, FVM_Context *source_context, FVM_Object *source) /* Borrow owners. */
{ /* Keep every partial acquisition cleanup-safe. */
    FVM_Context *destination = 0; /* Own a distinct heap until this helper returns. */
    FVM_Channel *queue = 0; /* Own one in-process FIFO handle. */
    FVM_Value root, copy, args[2], out; /* Borrow/copy tagged values synchronously. */
    int pinned = 0; /* [GC:] Track only a successfully acquired destination pin. */
    FVM_Result status = fvm_context_create(runtime, &destination); /* Share runtime, never source object pointers. */
    if (status == FVM_OK) /* Supply Object/Array support to the receiving heap. */
        status = fvm_standard_install(destination); /* Namespace installation is unnecessary for imported methods. */
    if (status == FVM_OK) /* Bind the sender's native callback to a stable application name. */
        status = fvm_serialization_register_callback(source_context, "example.broadcaster.receive", receive); /* Code identity only. */
    if (status == FVM_OK) /* The receiver independently provides its local implementation. */
        status = fvm_serialization_register_callback(destination, "example.broadcaster.receive", receive); /* Never serialize machine addresses. */
    if (status == FVM_OK) /* Create an unbounded empty memory queue. */
        status = fvm_channel_create(&queue); /* Release this handle after synchronous use. */
    if (status == FVM_OK) /* Capture the caller-pinned source graph. */
    { /* Publish no original VM pointers to the queue. */
        fvm_value_undefined(&root); /* Initialize every field of the borrowed root value. */
        root.type = FVM_VALUE_OBJECT; /* Select the managed graph representation. */
        root.object = source; /* Caller retains the original during preparation. */
        status = fvm_channel_send_value(source_context, queue, &root, 0); /* Nonblocking graph snapshot. */
    } /* End source capture. */
    if (status == FVM_OK) /* Import the owned snapshot into destination storage. */
        status = fvm_channel_receive_value(destination, queue, &copy, 0); /* Nonblocking; successful output belongs to destination. */
    if (status == FVM_OK) /* Retain the imported root across explicit collection. */
    { /* [GC:] Pin one destination object, which retains its whole listener graph. */
        status = fvm_gc_pin(destination, copy.object); /* [GC:] Local C values alone are not collector roots. */
        pinned = status == FVM_OK; /* Cleanup unpins only an acquired root. */
    } /* End destination pin acquisition. */
    if (status == FVM_OK) /* Demonstrate that the imported live Array keeps its listener reachable. */
        status = fvm_gc_collect(destination, 0); /* [GC:] Collect unreachable preparation objects before delivery. */
    if (status == FVM_OK) /* Use the imported callback bridge without source-heap access. */
    { /* Prepare a distinct event payload to make the copy visible. */
        status = fvm_value_string(args, "onValue"); /* Borrow a static event spelling. */
        fvm_value_number(args + 1, 84); /* The copied listener receives this Number. */
        if (status == FVM_OK) /* Dispatch synchronously in the destination owner's thread. */
            status = fvm_object_call(destination, copy.object, "broadcastMessage", 2, args, &out); /* Expected output: onValue: 84. */
    } /* End copied event dispatch. */
    if (pinned) /* End native access before releasing this destination root. */
        fvm_gc_unpin(destination, copy.object); /* [GC:] Context destruction releases all remaining cloned state. */
    if (queue) /* Also drop any still-queued packet after a partial failure. */
        fvm_channel_release(queue); /* Last handle owns FIFO cleanup. */
    if (destination) /* Source remains borrowed and untouched by destination destruction. */
        fvm_context_destroy(destination); /* End all imported object lifetimes. */
    return status; /* Caller preserves the first failure and releases the original graph. */
} /* No destination pointer escapes this helper. */
/* Demonstrate construction, registration, dispatch and removal on the VM owner thread. */
int main(void) /* Compile as strict C89; no display SDK or hardware is needed. */
{ /* Initialize native ownership before acquiring VM resources. */
    FVM_Config config; /* Configuration storage belongs to this stack frame. */
    FVM_Runtime *runtime = 0; /* Destroy after its context. */
    FVM_Context *context = 0; /* Own every managed object below. */
    FVM_Object *source = 0, *listener = 0; /* Borrow while pinned. */
    FVM_Value args[2], out; /* Borrowed call inputs and copied immediate output. */
    int source_pin = 0, listener_pin = 0; /* [GC:] Track only successfully acquired pins. */
    FVM_Result status = fvm_config_init(&config); /* Initialize ABI defaults. */
    if (status == FVM_OK) /* Acquire the outer owner first. */
        status = fvm_runtime_create(&config, &runtime); /* Return an owned runtime. */
    if (status == FVM_OK) /* Create an independent heap. */
        status = fvm_context_create(runtime, &context); /* Return an owned context. */
    if (status == FVM_OK) /* Prepare Object/Array support. */
        status = fvm_standard_install(context); /* Do not implicitly select an event frontend. */
    if (status == FVM_OK) /* Expose AsBroadcaster.initialize for future script callers. */
        status = fvm_standard_broadcaster_install(context); /* Native initialization also works without this namespace. */
    if (status == FVM_OK) /* Create the application's event source. */
        status = fvm_object_create(context, &source); /* The context owns its allocation. */
    if (status == FVM_OK) /* Protect source construction. */
    { /* [GC:] Acquire one temporary native root. */
        status = fvm_gc_pin(context, source); /* [GC:] Retain source until cleanup. */
        source_pin = status == FVM_OK; /* Record acquisition success. */
    } /* End source pin setup. */
    if (status == FVM_OK) /* Add native methods and an initially empty live Array. */
        status = fvm_broadcaster_initialize(context, source); /* Existing unrelated slots are preserved. */
    if (status == FVM_OK) /* Create an independent listener identity. */
        status = fvm_object_create(context, &listener); /* This object receives onValue. */
    if (status == FVM_OK) /* Protect receiver setup and later explicit removal. */
    { /* [GC:] Acquire a root separate from registration ownership. */
        status = fvm_gc_pin(context, listener); /* [GC:] The pin remains until the last native access. */
        listener_pin = status == FVM_OK; /* Do not unpin a failed acquisition. */
    } /* End receiver pin setup. */
    if (status == FVM_OK) /* Install the application event handler. */
        status = fvm_object_define_method_rooted(context, listener, "onValue", receive, 0, 0); /* No userdata or capture. */
    if (status == FVM_OK) /* Register the listener through the same method used by ActionScript. */
    { /* Prepare one same-context object argument. */
        fvm_value_undefined(args); /* Initialize every value field before assigning an object. */
        args[0].type = FVM_VALUE_OBJECT; /* Select the managed object representation. */
        args[0].object = listener; /* Borrow the pinned listener identity. */
        status = fvm_object_call(context, source, "addListener", 1, args, &out); /* The live Array retains this identity. */
    } /* End registration. */
    if (status == FVM_OK) /* Prepare a named event and one numeric payload. */
    { /* Native calls execute immediately on this same owning thread. */
        status = fvm_value_string(args, "onValue"); /* Borrow a static terminated event name. */
        fvm_value_number(args + 1, 42); /* Copy a Number into the second call argument. */
        if (status == FVM_OK) /* Forward only the payload to each handler. */
            status = fvm_object_call(context, source, "broadcastMessage", 2, args, &out); /* Snapshot recipients before callbacks. */
    } /* End event dispatch. */
    if (status == FVM_OK) /* Demonstrate portable graph state between independent heaps. */
        status = deliver_copy(runtime, context, source); /* Print onValue: 84 through the cloned listener. */
    if (status == FVM_OK) /* Remove registration explicitly while both objects remain pinned. */
    { /* Reuse call storage only after synchronous delivery has completed. */
        fvm_value_undefined(args); /* Clear the former borrowed string representation. */
        args[0].type = FVM_VALUE_OBJECT; /* Prepare an identity argument. */
        args[0].object = listener; /* Select the registered object. */
        status = fvm_object_call(context, source, "removeListener", 1, args, &out); /* Return true for our existing registration. */
        if (status == FVM_OK && (out.type != FVM_VALUE_BOOLEAN || !out.boolean)) /* Check normal protocol results. */
            status = FVM_ERROR; /* Report unexpected state without an assertion. */
    } /* End removal. */
    if (status != FVM_OK) /* Preserve the original runtime failure for the user. */
        fprintf(stderr, "Broadcaster example failed: %d\n", (int)status); /* Cleanup also handles partial setup. */
    if (listener_pin) /* No native receiver access remains after this point. */
        fvm_gc_unpin(context, listener); /* [GC:] Drop the temporary receiver root. */
    if (source_pin) /* Release the application's temporary ownership. */
        fvm_gc_unpin(context, source); /* [GC:] Remaining graph storage belongs to context teardown. */
    if (context) /* Also release registration left after an earlier error. */
        fvm_context_destroy(context); /* End every borrowed VM pointer lifetime. */
    if (runtime) /* Destroy the outer owner last. */
        fvm_runtime_destroy(runtime); /* Finish native cleanup. */
    return status == FVM_OK ? 0 : 1; /* Expose success or failure to the shell. */
} /* End one synchronous embedding session. */
