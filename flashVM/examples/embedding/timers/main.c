/* FlashVM C89 tutorial: retain a callback, explicitly poll and cancel an interval. */
#include "flashvm/standard.h" /* Runtime acquisition and built-in globals; see README.md API table. */
#include "flashvm/function.h" /* Construct a context-owned native Function with borrowed host state. */
#include "flashvm/stage.h"    /* Observe/consume coalesced redraw requests without rendering recursively. */
#include "flashvm/timers.h"   /* Explicit synchronous delivery on this context's owning thread. */
#include "tutorial_limits.h"  /* Give the human-visible timer delay a local semantic name. */
#include <stdio.h>            /* Print deliveries and optionally wait for a human event. */
#include <string.h>           /* Recognize the optional interactive mode without any platform SDK. */


/* Count deliveries; the ordinary native callback barrier remains active during this function. */
static FVM_Result tick(FVM_Context *context, FVM_Object *receiver, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw) /* README.md documents borrowed userdata. */
{                                                                                                                                        /* Run synchronously on the polling VM thread. */
    unsigned int *calls = (unsigned int *)raw;                                                                                           /* Host storage remains alive through context destruction. */
    (void)receiver;                                                                                                                      /* This tutorial needs no receiver or callback arguments. */
    (void)count;                                                                                                                         /* This tutorial needs no receiver or callback arguments. */
    (void)args;                                                                                                                          /* This tutorial needs no receiver or callback arguments. */
    ++*calls;                                                                                                                            /* Record only callbacks actually attempted by the host poll. */
    return fvm_call(context, "updateAfterEvent", 0, 0, out);                                                                             /* The interval call chain is eligible; the result is undefined. */
} /* Returning does not create another OS thread or keep an active call frame. */


/* Default mode polls once; --wait permits a human-driven event loop without slowing CTest. */
int main(int argc, char **argv)                         /* Build as C89 with the FlashVM static library. */
{                                                       /* Initialize partial ownership before acquisition. */
    FVM_Config config;                                  /* Configuration is copied into its runtime. */
    FVM_Runtime *runtime = 0;                           /* Outlive the managed heap. */
    FVM_Context *context = 0;                           /* Own callback objects and pending timer records. */
    FVM_Object *function = 0;                           /* Borrowed Function pointer retained by the timer after registration. */
    FVM_Value args[2], id, ignored;                     /* Prepare Function/delay and retain the plain numeric ID. */
    unsigned long wait_ms = 0;                          /* Receive a capped hint without invoking the callback. */
    int redraw = 0, observed_redraw = 0;                /* Plain host flags own no managed storage. */
    int pending = 0;                                    /* Distinguish an empty queue from a timer at the wait ceiling. */
    unsigned int calls = 0, delivered = 0;              /* Host-owned counters; no cross-thread access occurs. */
    FVM_Result status = fvm_config_init(&config);       /* Initialize the public configuration ABI. */
    if (status == FVM_OK)                               /* Acquire the outer owner first. */
    {                                                   /* Begin conditional branch: Acquire the outer owner first. */
        status = fvm_runtime_create(&config, &runtime); /* No worker is started. */
    } /* End conditional branch: Acquire the outer owner first. */
    if (status == FVM_OK)                               /* Establish the independent heap and monotonic clock origin. */
    {                                                   /* Begin conditional branch: Establish the independent heap and monotonic clock origin. */
        status = fvm_context_create(runtime, &context); /* Use only from this owning thread. */
    } /* End conditional branch: Establish the independent heap and monotonic clock origin. */
    if (status == FVM_OK)                       /* Install scheduling globals alongside the standard objects. */
    {                                           /* Begin conditional branch: Install scheduling globals alongside the standard objects. */
        status = fvm_standard_install(context); /* Conflicting own names cause a reported error. */
    } /* End conditional branch: Install scheduling globals alongside the standard objects. */
    if (status == FVM_OK)                                                         /* Store callback code and borrowed host counters in a managed Function. */
    {                                                                             /* Begin conditional branch: Store callback code and borrowed host counters in a managed Function. */
        status = fvm_function_create_native(context, tick, &calls, 0, &function); /* No capture object is needed. */
    } /* End conditional branch: Store callback code and borrowed host counters in a managed Function. */
    if (status == FVM_OK)                                        /* Prepare the two registration arguments without an intervening GC. */
    {                                                            /* Publish a complete Function value before scheduling it. */
        fvm_value_undefined(&args[0]);                           /* Initialize every unused value field. */
        args[0].type = FVM_VALUE_OBJECT;                         /* Reference the same-context Function. */
        args[0].object = function;                               /* Reference the same-context Function. */
        fvm_value_number(&args[1], FVM_TUTORIAL_TIMER_DELAY_MS); /* Use the explicit millisecond delay. */
        status = fvm_call(context, "setInterval", 2, args, &id); /* [GC:] Pending registration now retains the callback. */
    } /* The numeric ID owns no managed storage. */
    if (status == FVM_OK)                                                                            /* Query after registration; the host remains responsible for waiting. */
    {                                                                                                /* Begin conditional branch: Query after registration; the host remains responsible for waiting. */
        status = fvm_timers_next_delay(context, FVM_TUTORIAL_TIMER_WAIT_CAP_MS, &wait_ms, &pending); /* See README.md and TIMER_WAKEUP.md. */
    } /* End conditional branch: Query after registration; the host remains responsible for waiting. */
    if (status == FVM_OK)                                                       /* Report a hint; querying does not deliver or consume the interval. */
    {                                                                           /* Begin conditional branch: Report a hint; querying does not deliver or consume the interval. */
        printf("next poll in at most %lu ms (pending=%d)\n", wait_ms, pending); /* The hint can reach zero if setup was slow. */
    } /* End conditional branch: Report a hint; querying does not deliver or consume the interval. */
    if (status == FVM_OK && argc > 1 && strcmp(argv[1], "--wait") == 0)  /* Waiting is an optional host event, outside VM execution. */
    {                                                                    /* The default noninteractive run never blocks for input. */
        printf("Wait at least one second, then press Enter to poll.\n"); /* Explain when the callback becomes due. */
        (void)getchar();                                                 /* Replace with the application's ordinary event source in an integration. */
    } /* The VM creates neither a sleep nor a background timer. */
    if (status == FVM_OK)                              /* Attempt each currently due callback at most once. */
    {                                                  /* Begin conditional branch: Attempt each currently due callback at most once. */
        status = fvm_timers_poll(context, &delivered); /* A late interval is coalesced rather than replayed in a burst. */
    } /* End conditional branch: Attempt each currently due callback at most once. */
    if (status == FVM_OK)                                         /* Reading the pending flag must not consume a callback's request. */
    {                                                             /* Begin conditional branch: Reading the pending flag must not consume a callback's request. */
        status = fvm_stage_get_update(context, &observed_redraw); /* Owner-thread read without callbacks or allocations. */
    } /* End conditional branch: Reading the pending flag must not consume a callback's request. */
    if (status == FVM_OK)                                 /* Consume before the host would redraw; later requests remain distinguishable. */
    {                                                     /* Begin conditional branch: Consume before the host would redraw; later requests remain distinguishable. */
        status = fvm_stage_take_update(context, &redraw); /* This example reports the flag without requiring a display. */
    } /* End conditional branch: Consume before the host would redraw; later requests remain distinguishable. */
    if (status == FVM_OK && (redraw != observed_redraw || redraw != (calls ? 1 : 0))) /* Match one coalesced flag to actual interval delivery. */
    {                                                                                 /* Begin conditional branch: Match one coalesced flag to actual interval delivery. */
        status = FVM_ERROR;                                                           /* Preserve ordinary cleanup on an unexpected protocol result. */
    } /* End conditional branch: Match one coalesced flag to actual interval delivery. */
    if (status == FVM_OK)                        /* A real graphics host would now render once when this value is one. */
    {                                            /* Begin conditional branch: A real graphics host would now render once when this value is one. */
        printf("callback redraw: %d\n", redraw); /* Normally zero by default, one after the optional wait. */
    } /* End conditional branch: A real graphics host would now render once when this value is one. */
    if (status == FVM_OK)                                              /* Release the pending callback root explicitly when finished. */
    {                                                                  /* Begin conditional branch: Release the pending callback root explicitly when finished. */
        status = fvm_call(context, "clearInterval", 1, &id, &ignored); /* Cancellation returns undefined. */
    } /* End conditional branch: Release the pending callback root explicitly when finished. */
    if (status == FVM_OK)                                                                            /* An empty queue returns the host ceiling without reading the clock. */
    {                                                                                                /* Begin conditional branch: An empty queue returns the host ceiling without reading the clock. */
        status = fvm_timers_next_delay(context, FVM_TUTORIAL_TIMER_WAIT_CAP_MS, &wait_ms, &pending); /* Cancellation removes the pending root. */
    } /* End conditional branch: An empty queue returns the host ceiling without reading the clock. */
    if (status == FVM_OK && (pending || wait_ms != FVM_TUTORIAL_TIMER_WAIT_CAP_MS)) /* Validate the empty-queue contract with ordinary error handling. */
    {                                                                               /* Begin conditional branch: Validate the empty-queue contract with ordinary error handling. */
        status = FVM_ERROR;                                                         /* Preserve one cleanup path on an unexpected scheduler result. */
    } /* End conditional branch: Validate the empty-queue contract with ordinary error handling. */
    if (status == FVM_OK)                           /* Explicit host requests do not require an ActionScript event scope. */
    {                                               /* Begin conditional branch: Explicit host requests do not require an ActionScript event scope. */
        status = fvm_stage_request_update(context); /* Requesting twice still records only one pending redraw. */
    } /* End conditional branch: Explicit host requests do not require an ActionScript event scope. */
    if (status == FVM_OK)                           /* Demonstrate coalescing without invoking the native timer callback again. */
    {                                               /* Begin conditional branch: Demonstrate coalescing without invoking the native timer callback again. */
        status = fvm_stage_request_update(context); /* No allocation, callback or presentation occurs. */
    } /* End conditional branch: Demonstrate coalescing without invoking the native timer callback again. */
    if (status == FVM_OK)                                 /* Consume that single host request before any real presentation. */
    {                                                     /* Begin conditional branch: Consume that single host request before any real presentation. */
        status = fvm_stage_take_update(context, &redraw); /* A host retry after presentation failure would request again. */
    } /* End conditional branch: Consume that single host request before any real presentation. */
    if (status == FVM_OK)                                         /* No second request remains after the coalesced flag is consumed. */
    {                                                             /* Begin conditional branch: No second request remains after the coalesced flag is consumed. */
        status = fvm_stage_get_update(context, &observed_redraw); /* The output must be zero. */
    } /* End conditional branch: No second request remains after the coalesced flag is consumed. */
    if (status == FVM_OK && (redraw != 1 || observed_redraw != 0)) /* Verify explicit-host coalescing through ordinary checks. */
    {                                                              /* Begin conditional branch: Verify explicit-host coalescing through ordinary checks. */
        status = FVM_ERROR;                                        /* Do not silently accept repeated queued redraws. */
    } /* End conditional branch: Verify explicit-host coalescing through ordinary checks. */
    if (status == FVM_OK)                                                               /* Report actual timing rather than requiring an exact platform clock sample. */
    {                                                                                   /* Begin conditional branch: Report actual timing rather than requiring an exact platform clock sample. */
        printf("cancelled interval; callbacks: %u; attempted: %u\n", calls, delivered); /* Normally 0 by default, 1 after --wait. */
    } /* End conditional branch: Report actual timing rather than requiring an exact platform clock sample. */
    else                                                            /* Preserve clock, callback, allocation and budget errors. */
    {                                                               /* Begin conditional branch: Preserve clock, callback, allocation and budget errors. */
        fprintf(stderr, "timer example failed: %d\n", (int)status); /* Cleanup also releases a still-pending registration. */
    } /* End conditional branch: Preserve clock, callback, allocation and budget errors. */
    if (context)                      /* End use of the callback and host userdata before returning from main. */
    {                                 /* Begin conditional branch: End use of the callback and host userdata before returning from main. */
        fvm_context_destroy(context); /* [GC:] Release pending records and all managed objects. */
    } /* End conditional branch: End use of the callback and host userdata before returning from main. */
    if (runtime)                      /* Release the external owner after its context. */
    {                                 /* Begin conditional branch: Release the external owner after its context. */
        fvm_runtime_destroy(runtime); /* No timer thread remains to join. */
    } /* End conditional branch: Release the external owner after its context. */
    return status == FVM_OK ? 0 : 1; /* Expose integration failures to the shell or CTest. */
} /* Stack counters outlive every possible callback in this example. */
