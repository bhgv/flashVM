/* Freeze queue delivery, inspect a message without dequeueing and then resume normal receipt. */
#include "../support.h"       /* Provide checked VM ownership and ordinary error output. */
#include "flashvm/debug.h"    /* Declare channel freeze, metadata and nondestructive message inspection. */
#include "flashvm/transfer.h" /* Declare context-owned graph/value channel transfers. */


/* Keep this tutorial nonblocking so an inspection mistake cannot hang the host. */
int main(void)                              /* No external process or network connection is required. */
{                                           /* Begin the queue-debugging example. */
    FVM_Runtime *runtime = 0;               /* Own the local runtime until all channel operations finish. */
    FVM_Context *context = 0;               /* Own values decoded by debugger inspection and ordinary receipt. */
    FVM_Channel *channel = 0;               /* Own a queue handle independently of the VM context. */
    FVM_DebugChannel state;                 /* Receive a copied queue metadata snapshot. */
    FVM_Value sent, peeked, received;       /* Keep sent, inspected and consumed values distinct. */
    FVM_Result status;                      /* Distinguish a deliberately frozen receive from an ordinary failure. */
    int exit_code = 1;                      /* Return success only after the complete freeze/inspect/resume sequence. */
    if (!debugger_open(&runtime, &context)) /* Create an isolated VM for decoded inspection values. */
    {                                       /* Begin conditional branch: Create an isolated VM for decoded inspection values. */
        goto cleanup;                       /* Release partial VM ownership on setup failure. */
    } /* End conditional branch: Create an isolated VM for decoded inspection values. */
    if (!debugger_ok(fvm_channel_create(&channel), "channel_create")) /* Acquire one in-process FIFO queue. */
    {                                                                 /* Begin conditional branch: Acquire one in-process FIFO queue. */
        goto cleanup;                                                 /* Leave the channel handle empty on failure. */
    } /* End conditional branch: Acquire one in-process FIFO queue. */
    fvm_value_number(&sent, 42);                                                                /* Send a primitive so the raw inspected value is easy to compare. */
    if (!debugger_ok(fvm_channel_send_value(context, channel, &sent, 0), "channel_send_value")) /* Enqueue without allowing the host to block on capacity. */
    {                                                                                           /* Begin conditional branch: Enqueue without allowing the host to block on capacity. */
        goto cleanup;                                                                           /* Keep a transfer failure visible before freezing anything. */
    } /* End conditional branch: Enqueue without allowing the host to block on capacity. */
    if (!debugger_ok(fvm_debug_channel_freeze(channel, 1), "debug_channel_freeze")) /* Stop delivery while leaving producers able to enqueue. */
    {                                                                               /* Begin conditional branch: Stop delivery while leaving producers able to enqueue. */
        goto cleanup;                                                               /* Avoid claiming that a failed freeze isolated delivery. */
    } /* End conditional branch: Stop delivery while leaving producers able to enqueue. */
    if (!debugger_ok(fvm_debug_channel_state(channel, &state), "debug_channel_state")) /* Copy pending-count and frozen-state metadata without consuming a record. */
    {                                                                                  /* Begin conditional branch: Copy pending-count and frozen-state metadata without consuming a record. */
        goto cleanup;                                                                  /* Do not inspect uninitialized snapshot fields. */
    } /* End conditional branch: Copy pending-count and frozen-state metadata without consuming a record. */
    fprintf(stdout, "Queue %lu: pending=%lu frozen=%d\n", state.identity, (unsigned long)state.pending, state.frozen); /* Display the paused queue state. */
    if (!debugger_ok(fvm_debug_channel_message(context, channel, 0, &peeked), "debug_channel_message"))                /* Decode an independent snapshot of the first unread message. */
    {                                                                                                                  /* Begin conditional branch: Decode an independent snapshot of the first unread message. */
        goto cleanup;                                                                                                  /* Leave the actual queue message untouched if inspection fails. */
    } /* End conditional branch: Decode an independent snapshot of the first unread message. */
    fprintf(stdout, "Inspected value: %.17g\n", peeked.number);         /* Read this fixture's numeric value without user coercion. */
    status = fvm_channel_receive_value(context, channel, &received, 0); /* Attempt a nonblocking ordinary receive while delivery is frozen. */
    if (status != FVM_NOT_FOUND)                                        /* A frozen open queue must not deliver the pending message here. */
    {                                                                   /* Report a violated expectation without aborting the process. */
        fprintf(stderr, "Frozen receive returned %d\n", (int)status);   /* Preserve the actual queue result. */
        goto cleanup;                                                   /* Release the channel only after every operation has returned. */
    } /* Finish checking frozen delivery. */
    if (!debugger_ok(fvm_debug_channel_freeze(channel, 0), "debug_channel_resume")) /* Re-enable ordinary queue delivery explicitly. */
    {                                                                               /* Begin conditional branch: Re-enable ordinary queue delivery explicitly. */
        goto cleanup;                                                               /* Keep resume failures visible to the host. */
    } /* End conditional branch: Re-enable ordinary queue delivery explicitly. */
    if (!debugger_ok(fvm_channel_receive_value(context, channel, &received, 0), "channel_receive_value")) /* Consume the same message after the debugger releases delivery. */
    {                                                                                                     /* Begin conditional branch: Consume the same message after the debugger releases delivery. */
        goto cleanup;                                                                                     /* Avoid reading output from a failed receive. */
    } /* End conditional branch: Consume the same message after the debugger releases delivery. */
    if (received.type != FVM_VALUE_NUMBER || received.number != 42)                        /* Verify that inspection did not replace or consume the message. */
    {                                                                                      /* Handle a mismatched result through normal control flow. */
        fprintf(stderr, "The resumed message differs from the queued numeric fixture.\n"); /* Explain the expected invariant. */
        goto cleanup;                                                                      /* Keep the tutorial unsuccessful after a data mismatch. */
    } /* Finish checking nondestructive message inspection. */
    exit_code = 0;                    /* The queue was frozen, inspected and resumed successfully. */
cleanup:                              /* Join all channel and VM ownership paths. */
    if (channel)                      /* Creation may have failed before a handle was acquired. */
    {                                 /* Begin conditional branch: Creation may have failed before a handle was acquired. */
        fvm_channel_destroy(channel); /* Release the queue after its synchronous users have finished. */
    } /* End conditional branch: Creation may have failed before a handle was acquired. */
    debugger_close(runtime, context); /* Release decoded values and the owning runtime last. */
    return exit_code;                 /* Return the checked result of the queue debugger example. */
} /* End the channel interception tutorial. */
