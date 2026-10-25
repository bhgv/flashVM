/* Deliver explicit signed wheel lines without an OS wheel; see MOUSE.md#wheel-events. */
#include "flashvm/mouse.h"       /* Register receivers and dispatch finite wheel deltas on the owner thread. */
#include "flashvm/input_queue.h" /* Copy wheel events into the shared owner-thread FIFO. */
#include "flashvm/input.h"       /* Read the callback-visible world coordinates. */
#include "flashvm/standard.h"    /* Install ordinary objects before the optional Mouse namespace. */
#include "flashvm/gc.h"          /* Pin partial setup until the registration owns the receiver. */
#include "flashvm/value.h"       /* Define synchronous receiver-bound native handlers. */
#include <stdio.h>               /* Report normal results and runtime failures. */


/* Borrow the host counter until unregistration; MOUSE.md#delivery defines event lifetime. */
static FVM_Result receive(FVM_Context *context, FVM_Object *receiver, unsigned int count, /* Borrow owners. */
                          const FVM_Value *args, FVM_Value *out, void *raw)               /* Borrow call storage. */
{                                                                                         /* Begin one synchronous listener call. */
    unsigned int *calls = (unsigned int *)raw;                                            /* Main owns this counter through context teardown. */
    double x, y;                                                                          /* Receive a copied native coordinate pair. */
    FVM_Result status;                                                                    /* Preserve the real query status. */
    (void)receiver;                                                                       /* Registered identity receives its own onMouseWheel method. */
    if (count != 1 || !args || args[0].type != FVM_VALUE_NUMBER)                          /* Validate the documented delta payload. */
    {                                                                                     /* Begin conditional branch: Validate the documented delta payload. */
        return FVM_INVALID_ARGUMENT;                                                      /* Ordinary error handling, never an assertion in a tutorial. */
    } /* End conditional branch: Validate the documented delta payload. */
    status = fvm_input_get_pointer(context, &x, &y); /* No allocation or OS query occurs. */
    if (status != FVM_OK)                            /* Propagate the original error after pointer publication. */
    {                                                /* Begin conditional branch: Propagate the original error after pointer publication. */
        return status;                               /* The dispatcher preserves committed coordinates. */
    } /* End conditional branch: Propagate the original error after pointer publication. */
    ++*calls;                                                                    /* Count short contacts even if no movie frame occurs between events. */
    printf("wheel %u: delta=%.1f at %.0f,%.0f\n", *calls, args[0].number, x, y); /* One Number argument. */
    fvm_value_undefined(out);                                                    /* Return values do not alter event delivery. */
    return FVM_OK;                                                               /* Permit the next receiver to run. */
} /* End the borrowed callback lifetime. */


/* Run a strict-C89 embedding host; no window, permission or physical input is required. */
int main(void)                                          /* Use one owning thread for every VM operation. */
{                                                       /* Acquire owners in dependency order. */
    FVM_Config config;                                  /* Own configuration storage. */
    FVM_Runtime *runtime = 0;                           /* Release after the context. */
    FVM_Context *context = 0;                           /* Own all managed receiver and namespace state. */
    FVM_Object *listener = 0;                           /* Borrow while pinned or registered. */
    unsigned int pending = 0, processed = 0;            /* Queue counts are native outputs, not managed objects. */
    unsigned int calls = 0;                             /* Borrowed userdata remains live until the context is destroyed. */
    int pinned = 0, registered = 0, removed = 0;        /* Track acquired resources for partial cleanup. */
    FVM_Result status = fvm_config_init(&config);       /* Initialize the supported configuration layout. */
    if (status == FVM_OK)                               /* Acquire the runtime only after successful configuration. */
    {                                                   /* Begin conditional branch: Acquire the runtime only after successful configuration. */
        status = fvm_runtime_create(&config, &runtime); /* Return an owned runtime. */
    } /* End conditional branch: Acquire the runtime only after successful configuration. */
    if (status == FVM_OK)                               /* Keep context state separate from every other embedded movie. */
    {                                                   /* Begin conditional branch: Keep context state separate from every other embedded movie. */
        status = fvm_context_create(runtime, &context); /* Return an owned VM context. */
    } /* End conditional branch: Keep context state separate from every other embedded movie. */
    if (status == FVM_OK)                       /* Prepare ordinary Object and Array support first. */
    {                                           /* Begin conditional branch: Prepare ordinary Object and Array support first. */
        status = fvm_standard_install(context); /* Do not implicitly install input namespaces. */
    } /* End conditional branch: Prepare ordinary Object and Array support first. */
    if (status == FVM_OK)                             /* Make Mouse.addListener/removeListener available to a future movie. */
    {                                                 /* Begin conditional branch: Make Mouse.addListener/removeListener available to a future movie. */
        status = fvm_standard_mouse_install(context); /* Preserve any conflicting global binding. */
    } /* End conditional branch: Make Mouse.addListener/removeListener available to a future movie. */
    if (status == FVM_OK)                               /* Create a receiver before constructing its native methods. */
    {                                                   /* Begin conditional branch: Create a receiver before constructing its native methods. */
        status = fvm_object_create(context, &listener); /* Borrow the result from the context. */
    } /* End conditional branch: Create a receiver before constructing its native methods. */
    if (status == FVM_OK)                       /* Protect incomplete setup before registration can retain it. */
    {                                           /* Begin temporary root acquisition. */
        status = fvm_gc_pin(context, listener); /* [GC:] Keep the receiver graph alive. */
        pinned = status == FVM_OK;              /* Cleanup releases only an acquired pin. */
    } /* End temporary root acquisition. */
    if (status == FVM_OK)                                                                                /* Define only the wheel handler; down/move/up remain independent. */
    {                                                                                                    /* Begin conditional branch: Define only the wheel handler; down/move/up remain independent. */
        status = fvm_object_define_method_rooted(context, listener, "onMouseWheel", receive, &calls, 0); /* Native userdata. */
    } /* End conditional branch: Define only the wheel handler; down/move/up remain independent. */
    if (status == FVM_OK)                                         /* Register one strong context-owned identity. */
    {                                                             /* Track registration for explicit cleanup. */
        status = fvm_input_add_mouse_listener(context, listener); /* Duplicate registration would be harmless. */
        registered = status == FVM_OK;                            /* A failed allocation adds nothing. */
    } /* End registration acquisition. */
    if (status == FVM_OK)                                              /* Host has already normalized its native event into scroll lines. */
    {                                                                  /* Begin conditional branch: Host has already normalized its native event into scroll lines. */
        status = fvm_input_dispatch_mouse_wheel(context, 3.0, 10, 20); /* Positive means upward scroll. */
    } /* End conditional branch: Host has already normalized its native event into scroll lines. */
    if (status == FVM_OK)                                               /* Fractional smooth input is preserved, not truncated to integer ticks. */
    {                                                                   /* Begin conditional branch: Fractional smooth input is preserved, not truncated to integer ticks. */
        status = fvm_input_dispatch_mouse_wheel(context, -1.5, 30, 40); /* Negative means downward scroll. */
    } /* End conditional branch: Fractional smooth input is preserved, not truncated to integer ticks. */
    if (status == FVM_OK)                                              /* Queue captures the delta and position, without immediate delivery. */
    {                                                                  /* Begin conditional branch: Queue captures the delta and position, without immediate delivery. */
        status = fvm_input_enqueue_mouse_wheel(context, -0.5, 50, 60); /* Shares order with Key/Mouse/gestures. */
    } /* End conditional branch: Queue captures the delta and position, without immediate delivery. */
    if (status == FVM_OK)                              /* Inspect the copied FIFO before consuming it. */
    {                                                  /* Begin conditional branch: Inspect the copied FIFO before consuming it. */
        status = fvm_input_pending(context, &pending); /* Expect one pending event and still two callbacks. */
    } /* End conditional branch: Inspect the copied FIFO before consuming it. */
    if (status == FVM_OK)                                   /* Show the capture/delivery boundary in ordinary tutorial output. */
    {                                                       /* Begin conditional branch: Show the capture/delivery boundary in ordinary tutorial output. */
        printf("queued=%u delivered=%u\n", pending, calls); /* Expect queued=1 delivered=2. */
    } /* End conditional branch: Show the capture/delivery boundary in ordinary tutorial output. */
    if (status == FVM_OK)                                /* Deliver only the entry batch, on this same VM owner thread. */
    {                                                    /* Begin conditional branch: Deliver only the entry batch, on this same VM owner thread. */
        status = fvm_input_pump(context, 0, &processed); /* Zero limit means the initial queued batch. */
    } /* End conditional branch: Deliver only the entry batch, on this same VM owner thread. */
    if (status == FVM_OK && processed != 1) /* A successful pump must consume our one valid event. */
    {                                       /* Begin conditional branch: A successful pump must consume our one valid event. */
        status = FVM_ERROR;                 /* Surface unexpected protocol state without an assertion. */
    } /* End conditional branch: A successful pump must consume our one valid event. */
    if (registered)                                                                        /* Release the registration on success and every later failure. */
    {                                                                                      /* Removal itself performs no allocation or callback. */
        FVM_Result cleanup = fvm_input_remove_mouse_listener(context, listener, &removed); /* Remove by identity. */
        if (status == FVM_OK)                                                              /* Preserve an earlier runtime error if there was one. */
        {                                                                                  /* Begin conditional branch: Preserve an earlier runtime error if there was one. */
            status = cleanup;                                                              /* Otherwise expose cleanup failure. */
        } /* End conditional branch: Preserve an earlier runtime error if there was one. */
    } /* End registration cleanup. */
    if (status == FVM_OK)                                    /* Confirm the complete tutorial result with ordinary error handling. */
    {                                                        /* Begin conditional branch: Confirm the complete tutorial result with ordinary error handling. */
        printf("delivered=%u removed=%d\n", calls, removed); /* Expect three and one. */
    } /* End conditional branch: Confirm the complete tutorial result with ordinary error handling. */
    else                                                            /* Keep the real status visible to the embedding developer. */
    {                                                               /* Begin conditional branch: Keep the real status visible to the embedding developer. */
        fprintf(stderr, "Wheel example failed: %d\n", (int)status); /* Do not hide errors behind assertions. */
    } /* End conditional branch: Keep the real status visible to the embedding developer. */
    if (pinned)                          /* Release construction ownership after the last receiver access. */
    {                                    /* Begin conditional branch: Release construction ownership after the last receiver access. */
        fvm_gc_unpin(context, listener); /* [GC:] Remaining managed storage belongs to context teardown. */
    } /* End conditional branch: Release construction ownership after the last receiver access. */
    if (context)                      /* Also release any partial setup left by allocation failure. */
    {                                 /* Begin conditional branch: Also release any partial setup left by allocation failure. */
        fvm_context_destroy(context); /* End every borrowed object lifetime. */
    } /* End conditional branch: Also release any partial setup left by allocation failure. */
    if (runtime)                      /* Destroy the outer owner last. */
    {                                 /* Begin conditional branch: Destroy the outer owner last. */
        fvm_runtime_destroy(runtime); /* Finish native cleanup. */
    } /* End conditional branch: Destroy the outer owner last. */
    return status == FVM_OK ? 0 : 1; /* Report normal shell success/error. */
} /* End the complete ownership example. */
