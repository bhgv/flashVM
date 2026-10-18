/* Deliver a primary-contact sequence without an OS mouse; see MOUSE.md#native-host-contract. */
#include "flashvm/mouse.h" /* Register receivers and dispatch down/move/up on the owner thread. */
#include "flashvm/input.h" /* Read the callback-visible world coordinates. */
#include "flashvm/standard.h" /* Install ordinary objects before the optional Mouse namespace. */
#include "flashvm/gc.h" /* Pin partial setup until the registration owns the receiver. */
#include "flashvm/value.h" /* Define synchronous receiver-bound native handlers. */
#include <stdio.h> /* Report normal results and runtime failures. */
/* Borrow the host counter until unregistration; MOUSE.md#delivery defines event lifetime. */
static FVM_Result receive(FVM_Context *context, FVM_Object *receiver, unsigned int count, /* Borrow owners. */
                          const FVM_Value *args, FVM_Value *out, void *raw) /* Borrow call storage. */
{ /* Begin one synchronous listener call. */
    unsigned int *calls = (unsigned int *)raw; /* Main owns this counter through context teardown. */
    double x, y; /* Receive a copied native coordinate pair. */
    FVM_Result status; /* Preserve the real query status. */
    (void)receiver; /* The same listener receives all three methods in this tutorial. */
    (void)count; /* Pointer listeners receive no positional arguments. */
    (void)args; /* Read position through the native C89 query instead. */
    status = fvm_input_get_pointer(context, &x, &y); /* No allocation or OS query occurs. */
    if (status != FVM_OK) /* Propagate the original error after pointer publication. */
        return status; /* The dispatcher preserves committed coordinates. */
    ++*calls; /* Count short contacts even if no movie frame occurs between events. */
    printf("event %u: %.0f,%.0f\n", *calls, x, y); /* Expect three accepted positions. */
    fvm_value_undefined(out); /* Return values do not alter event delivery. */
    return FVM_OK; /* Permit the next receiver to run. */
} /* End the borrowed callback lifetime. */
/* Run a strict-C89 embedding host; no window, permission or physical input is required. */
int main(void) /* Use one owning thread for every VM operation. */
{ /* Acquire owners in dependency order. */
    FVM_Config config; /* Own configuration storage. */
    FVM_Runtime *runtime = 0; /* Release after the context. */
    FVM_Context *context = 0; /* Own all managed receiver and namespace state. */
    FVM_Object *listener = 0; /* Borrow while pinned or registered. */
    unsigned int calls = 0; /* Borrowed userdata remains live until the context is destroyed. */
    int pinned = 0, registered = 0, removed = 0; /* Track acquired resources for partial cleanup. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the supported configuration layout. */
    if (status == FVM_OK) /* Acquire the runtime only after successful configuration. */
        status = fvm_runtime_create(&config, &runtime); /* Return an owned runtime. */
    if (status == FVM_OK) /* Keep context state separate from every other embedded movie. */
        status = fvm_context_create(runtime, &context); /* Return an owned VM context. */
    if (status == FVM_OK) /* Prepare ordinary Object and Array support first. */
        status = fvm_standard_install(context); /* Do not implicitly install input namespaces. */
    if (status == FVM_OK) /* Make Mouse.addListener/removeListener available to a future movie. */
        status = fvm_standard_mouse_install(context); /* Preserve any conflicting global binding. */
    if (status == FVM_OK) /* Create a receiver before constructing its native methods. */
        status = fvm_object_create(context, &listener); /* Borrow the result from the context. */
    if (status == FVM_OK) /* Protect incomplete setup before registration can retain it. */
    { /* Begin temporary root acquisition. */
        status = fvm_gc_pin(context, listener); /* [GC:] Keep the receiver graph alive. */
        pinned = status == FVM_OK; /* Cleanup releases only an acquired pin. */
    } /* End temporary root acquisition. */
    if (status == FVM_OK) /* Map primary-button/contact begin to this handler. */
        status = fvm_object_define_method_rooted(context, listener, "onMouseDown", receive, &calls, 0); /* Borrow counter. */
    if (status == FVM_OK) /* Map primary-pointer movement to the same handler. */
        status = fvm_object_define_method_rooted(context, listener, "onMouseMove", receive, &calls, 0); /* No capture root. */
    if (status == FVM_OK) /* Map primary-button/contact end to the same handler. */
        status = fvm_object_define_method_rooted(context, listener, "onMouseUp", receive, &calls, 0); /* Keep userdata live. */
    if (status == FVM_OK) /* Register one strong context-owned identity. */
    { /* Track registration for explicit cleanup. */
        status = fvm_input_add_mouse_listener(context, listener); /* Duplicate registration would be harmless. */
        registered = status == FVM_OK; /* A failed allocation adds nothing. */
    } /* End registration acquisition. */
    if (status == FVM_OK) /* Deliver a tap or stylus-down equivalent before a movie tick. */
        status = fvm_input_dispatch_mouse(context, FVM_MOUSE_EVENT_DOWN, 10, 20); /* Publish before callback. */
    if (status == FVM_OK) /* Deliver movement while the contact remains active. */
        status = fvm_input_dispatch_mouse(context, FVM_MOUSE_EVENT_MOVE, 30, 40); /* Update native drag first. */
    if (status == FVM_OK) /* Finish the same contact without requiring a physical mouse. */
        status = fvm_input_dispatch_mouse(context, FVM_MOUSE_EVENT_UP, 30, 40); /* Deliver the third callback. */
    if (registered) /* Release the registration on success and every later failure. */
    { /* Removal itself performs no allocation or callback. */
        FVM_Result cleanup = fvm_input_remove_mouse_listener(context, listener, &removed); /* Remove by identity. */
        if (status == FVM_OK) /* Preserve an earlier runtime error if there was one. */
            status = cleanup; /* Otherwise expose cleanup failure. */
    } /* End registration cleanup. */
    if (status == FVM_OK) /* Confirm the complete tutorial result with ordinary error handling. */
        printf("delivered=%u removed=%d\n", calls, removed); /* Expect three and one. */
    else /* Keep the real status visible to the embedding developer. */
        fprintf(stderr, "Mouse example failed: %d\n", (int)status); /* Do not hide errors behind assertions. */
    if (pinned) /* Release construction ownership after the last receiver access. */
        fvm_gc_unpin(context, listener); /* [GC:] Remaining managed storage belongs to context teardown. */
    if (context) /* Also release any partial setup left by allocation failure. */
        fvm_context_destroy(context); /* End every borrowed object lifetime. */
    if (runtime) /* Destroy the outer owner last. */
        fvm_runtime_destroy(runtime); /* Finish native cleanup. */
    return status == FVM_OK ? 0 : 1; /* Report normal shell success/error. */
} /* End the complete ownership example. */
