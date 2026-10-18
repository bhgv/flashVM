/* Register a native keyboard listener and deliver a complete tap before any movie tick. */
#include "flashvm/keyboard.h" /* Declare listener registration and snapshot dispatch. */
#include "flashvm/standard.h" /* Install standard objects before the explicit Key bridge. */
#include "flashvm/gc.h" /* Protect the listener during construction before registration. */
#include "flashvm/value.h" /* Define the receiver-bound native event methods. */
#include <stdio.h> /* Print deliveries and ordinary errors. */
#include <string.h> /* Initialize caller-owned snapshot storage. */
/* Read the already published state from inside either receiver-bound notification. */
static FVM_Result receive(FVM_Context *context, FVM_Object *self, /* Borrow VM and receiver. */
                          unsigned int count, const FVM_Value *args, /* Borrow argument storage. */
                          FVM_Value *out, void *raw) /* Borrow the result slot and native counter. */
{ /* Start one synchronous notification. */
    FVM_KeyboardState state; /* Own a temporary native copy of polling state. */
    FVM_Result status; /* Propagate failure instead of asserting. */
    unsigned int *deliveries = (unsigned int *)raw; /* Borrow main's counter until unregistration. */
    (void)self; /* This example does not need the listener receiver's properties. */
    (void)count; /* Key notifications carry no positional arguments. */
    (void)args; /* Query the published keyboard state instead. */
    state.struct_size = sizeof(state); /* Select the exact output layout. */
    status = fvm_input_get_keyboard(context, &state); /* Read without allocation or callback reentry. */
    if (status != FVM_OK) /* Preserve the API error for the dispatch caller. */
        return status; /* Main will clean up both owners. */
    ++*deliveries; /* Record both press and release even without an intervening frame. */
    printf("event %u: Left %s\n", *deliveries, state.down[FVM_KEY_LEFT] ? "down" : "up"); /* Show callback-visible state. */
    fvm_value_undefined(out); /* Event handler return values are ignored by delivery. */
    return FVM_OK; /* Continue to the next registered listener. */
} /* End the native event handler. */
/* Create the listener, dispatch a short tap and unregister before releasing borrowed userdata. */
int main(void) /* Run all VM operations on the same owning thread. */
{ /* Start the standalone C89 host. */
    FVM_Config config; /* Own default runtime configuration. */
    FVM_Runtime *runtime = 0; /* Acquire and destroy after the context. */
    FVM_Context *context = 0; /* Own native state and strong listener registrations. */
    FVM_Object *listener = 0; /* Borrow the managed object while pinned or registered. */
    FVM_KeyboardState state; /* Own the complete post-transition input snapshot. */
    unsigned int deliveries = 0; /* Keep native callback userdata alive through cleanup. */
    int pinned = 0, removed = 0; /* Track construction-root ownership and removal result. */
    FVM_Result status = fvm_config_init(&config); /* Initialize documented defaults. */
    if (status == FVM_OK) /* Do not acquire dependent resources after failure. */
        status = fvm_runtime_create(&config, &runtime); /* Acquire the allocation-policy owner. */
    if (status == FVM_OK) /* Create the isolated VM after its runtime. */
        status = fvm_context_create(runtime, &context); /* Acquire the context. */
    if (status == FVM_OK) /* Install ordinary standard objects first. */
        status = fvm_standard_install(context); /* Keep conflicts visible to the host. */
    if (status == FVM_OK) /* Open no OS keyboard; installation only publishes methods. */
        status = fvm_standard_keyboard_install(context); /* Make Key registration and polling available. */
    if (status == FVM_OK) /* Create a receiver for event callbacks. */
        status = fvm_object_create(context, &listener); /* Return a context-owned borrowed object. */
    if (status == FVM_OK) /* Protect the incomplete receiver before other allocating work. */
    { /* Begin construction-root acquisition. */
        status = fvm_gc_pin(context, listener); /* Add a temporary host GC root. */
        pinned = status == FVM_OK; /* Release it only when acquisition succeeded. */
    } /* Finish root acquisition. */
    if (status == FVM_OK) /* Bind a parameterless press callback to this receiver. */
        status = fvm_object_define_method_rooted(context, listener, "onKeyDown", /* Select press. */
                                                receive, &deliveries, 0); /* Borrow the native counter. */
    if (status == FVM_OK) /* The release callback reads the same published polling state. */
        status = fvm_object_define_method_rooted(context, listener, "onKeyUp", /* Select release. */
                                                receive, &deliveries, 0); /* Retain no managed capture. */
    if (status == FVM_OK) /* Publish a strong context registration once construction is complete. */
        status = fvm_input_add_key_listener(context, listener); /* Duplicate adds would be harmless. */
    memset(&state, 0, sizeof(state)); /* Initialize every held slot, code and toggle. */
    state.struct_size = sizeof(state); /* Select the exact ABI layout. */
    state.last_code = FVM_KEY_LEFT; /* Record the host's most recent pressed virtual key. */
    state.down[FVM_KEY_LEFT] = 1; /* Build the post-press polling state. */
    if (status == FVM_OK) /* State is published before the callback runs. */
        status = fvm_input_dispatch_keyboard(context, &state, 1); /* Deliver onKeyDown synchronously. */
    state.down[FVM_KEY_LEFT] = 0; /* Build the post-release state without changing the latest press code. */
    if (status == FVM_OK) /* Deliver release immediately, with no movie tick between events. */
        status = fvm_input_dispatch_keyboard(context, &state, 0); /* Deliver onKeyUp synchronously. */
    if (status == FVM_OK) /* Remove the registration before borrowed userdata leaves scope. */
        status = fvm_input_remove_key_listener(context, listener, &removed); /* Removed is one for this registered object. */
    if (status == FVM_OK) /* Show the cleanup result without tutorial assertions. */
        printf("removed=%d deliveries=%u\n", removed, deliveries); /* Expect one removal and two events. */
    if (status != FVM_OK) /* Report a real failure rather than a fabricated success. */
        fprintf(stderr, "keyboard listener failed: %d\n", (int)status); /* Preserve a useful status. */
    if (pinned) /* Release the temporary construction root before context destruction. */
        fvm_gc_unpin(context, listener); /* The context still owns the object storage. */
    if (context) /* Destroy registrations and objects on both success and failure. */
        fvm_context_destroy(context); /* End all potential use of the native counter. */
    if (runtime) /* Release the runtime after its context. */
        fvm_runtime_destroy(runtime); /* Finish owner cleanup. */
    return status == FVM_OK ? 0 : 1; /* Let the shell observe the final status. */
} /* End the standalone host. */
