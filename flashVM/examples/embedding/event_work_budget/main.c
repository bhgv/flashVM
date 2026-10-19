/* FlashVM C89 tutorial: a bounded pointer event keeps its committed position after an error. */
#include "flashvm/flashvm.h" /* Runtime and context ownership, statuses and work configuration. */
#include "flashvm/value.h" /* Managed object creation and native method registration. */
#include "flashvm/gc.h" /* Pins retain a temporary receiver during method construction. */
#include "flashvm/mouse.h" /* Native listener registration and explicit primary-pointer events. */
#include "flashvm/input.h" /* Read the position committed before callback delivery. */
#include <stdio.h> /* Print expected delivery and state; no input device is opened. */
#include "tutorial_limits.h" /* Private named bounds for this deterministic work example. */
/* Count a native callback without allocating, collecting or retaining its borrowed receiver. */
static FVM_Result receive(FVM_Context *context, FVM_Object *self, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* Runs synchronously in dispatch. */
{ /* The enclosing main frame owns the counter until context destruction. */
    unsigned int *calls = (unsigned int *)raw; /* Borrow stack storage, never transport it to a worker. */
    (void)context; (void)self; (void)count; (void)args; /* This observer needs no payload or managed access. */
    ++*calls; /* This completed side effect remains after the later budget failure. */
    fvm_value_undefined(out); /* Optional handlers ignore a successful return value. */
    return FVM_OK; /* Continue delivery while the enclosing event has work remaining. */
} /* End the borrowed callback scope. */
/* Configure a real runtime, register three listeners and observe a deliberately partial event. */
int main(void) /* Compile with a strict C89 compiler; no platform SDK or SWF is required. */
{ /* Own every resource acquired below and clean up on any failure. */
    FVM_Config config; /* Configuration is copied by runtime creation. */
    FVM_Runtime *runtime = 0; /* Outlive the context and its registered callbacks. */
    FVM_Context *context = 0; /* Own the managed heap and permanent registration roots. */
    FVM_Object *temporary = 0; /* Borrow one listener while it is under construction. */
    FVM_Result status; /* Preserve the first unexpected error for the exit status. */
    unsigned int i, calls = 0; /* Registration index and callback-owned scalar state. */
    int pinned = 0; /* [GC:] Track a successfully acquired temporary construction pin. */
    double x = 0, y = 0; /* Copied native coordinates remain usable without managed roots. */
    status = fvm_config_init(&config); /* Initialize ABI defaults before overriding one field. */
    if (status == FVM_OK) /* A numeric bound is copied into the new runtime. */
    { /* Set the work limit before creating its owner. */
        config.instruction_limit = EXAMPLE_WORK_LIMIT; /* Each independent native operation starts a new budget. */
        status = fvm_runtime_create(&config, &runtime); /* Acquire the outer owner. */
    } /* End configuration. */
    if (status == FVM_OK) /* A failed runtime acquisition needs no context cleanup. */
        status = fvm_context_create(runtime, &context); /* Acquire a separate heap on this thread. */
    for (i = 0; status == FVM_OK && i < EXAMPLE_LISTENER_COUNT; ++i) /* Each add is an independent native operation. */
    { /* Build one receiver before publishing its strong registration root. */
        status = fvm_object_create(context, &temporary); /* The new pointer alone is not a permanent root. */
        if (status == FVM_OK) /* [GC:] Protect construction until registration succeeds. */
        { /* [GC:] Cleanup tracks only successful pin acquisition. */
            status = fvm_gc_pin(context, temporary); /* [GC:] Keep the receiver alive through method setup. */
            pinned = status == FVM_OK; /* [GC:] Never unpin an unacquired pin. */
        } /* [GC:] End temporary root acquisition. */
        if (status == FVM_OK) /* Borrow calls until main releases the context. */
            status = fvm_object_define_method_rooted(context, temporary, "onMouseDown", receive, &calls, 0); /* No global handler fallback. */
        if (status == FVM_OK) /* Each add fits: one, three and five work units respectively. */
            status = fvm_input_add_mouse_listener(context, temporary); /* [GC:] The context now retains successful registration. */
        if (pinned) /* [GC:] Registration or cleanup has replaced the temporary root's purpose. */
        { /* [GC:] Release exactly the pin acquired in this iteration. */
            fvm_gc_unpin(context, temporary); /* [GC:] On failure the unused object is reclaimed with the context. */
            pinned = 0; /* [GC:] Prevent duplicate cleanup. */
        } /* [GC:] End temporary root lifetime. */
    } /* Finish registration or stop at the first unexpected error. */
    if (status == FVM_OK) /* All three receivers are retained by the context. */
    { /* Deliberately exhaust delivery after one completed observer. */
        status = fvm_input_dispatch_mouse(context, FVM_MOUSE_EVENT_DOWN, 23, 45); /* Capture three, publish, call one, fail before the second. */
        if (status == FVM_ERROR && calls == 1) /* This is the expected bounded outcome, not a retry request. */
            status = fvm_input_get_pointer(context, &x, &y); /* Publication remains committed despite delivery failure. */
        else /* Successful full delivery or another error violates this demonstration's expectation. */
            status = FVM_ERROR; /* Report unexpected behavior through ordinary cleanup. */
        if (status == FVM_OK && (x != 23 || y != 45)) /* Check copied coordinates without inspecting private VM state. */
            status = FVM_ERROR; /* A budget error must not undo published input. */
    } /* End the single explicit event. */
    if (status == FVM_OK) /* Print only the verified partial-delivery outcome. */
        printf("callbacks=%u pointer=%.0f,%.0f budget exhausted after publication\n", calls, x, y); /* Expected: callbacks=1 pointer=23,45. */
    else /* Preserve an observable failure for command-line callers. */
        fprintf(stderr, "event budget example failed: %d\n", (int)status); /* No swallowed failure or automatic replay. */
    if (context) /* Stop all access to receivers and their borrowed userdata before returning. */
        fvm_context_destroy(context); /* [GC:] Release registered listener graphs and any partial construction. */
    if (runtime) /* The runtime must outlive every associated context. */
        fvm_runtime_destroy(runtime); /* Release the outer owner after its heap. */
    return status == FVM_OK ? 0 : 1; /* Expose the verified result to scripts and CTest. */
} /* No borrowed object or userdata pointer escapes. */
