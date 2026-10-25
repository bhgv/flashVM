/* Inspect VM registries and coherent mutex ownership without creating a deadlock. */
#include "../support.h"    /* Reuse checked setup and explicit handle cleanup. */
#include "flashvm/debug.h" /* Declare registry and global resource snapshot APIs. */
#include "flashvm/mutex.h" /* Declare nonblocking mutex acquisition and owner-only release. */
#include <stdlib.h>        /* Allocate a caller-owned snapshot array after measuring required capacity. */


/* Show a local ownership row and explain why it is not a distributed deadlock proof. */
int main(void)                               /* Run the tutorial on one VM thread with no blocking acquisition. */
{                                            /* Begin the registry/resource debugger example. */
    FVM_Runtime *runtime = 0;                /* Own the runtime for this independent inspection session. */
    FVM_Context *context = 0;                /* Own the registry views and mutex owner identity. */
    FVM_Mutex *mutex = 0;                    /* Own a native mutex handle after successful creation. */
    FVM_DebugMutexDetail *rows = 0;          /* Own only the copied snapshot array, never registry internals. */
    size_t count = 0, needed = 0, index = 0; /* Keep registry counts, required capacity and traversal index separate. */
    const char *name = 0;                    /* Borrow one full registry name until the registry changes. */
    size_t name_size = 0;                    /* Preserve the counted name instead of silently truncating it. */
    FVM_Result status;                       /* Accept NOT_FOUND as the ordinary capacity-probe result. */
    int locked = 0;                          /* Release mutex ownership only after successful acquisition. */
    int exit_code = 1;                       /* Return success only after all checked inspection operations complete. */
    if (!debugger_open(&runtime, &context))  /* Create a VM containing inspectable standard classes. */
    {                                        /* Begin conditional branch: Create a VM containing inspectable standard classes. */
        goto cleanup;                        /* Release partial runtime/context initialization. */
    } /* End conditional branch: Create a VM containing inspectable standard classes. */
    if (!debugger_ok(fvm_debug_object_count(context, &count), "debug_object_count")) /* Count managed objects without rooting an entire heap snapshot. */
    {                                                                                /* Begin conditional branch: Count managed objects without rooting an entire heap snapshot. */
        goto cleanup;                                                                /* Keep an invalid count out of later output. */
    } /* End conditional branch: Count managed objects without rooting an entire heap snapshot. */
    fprintf(stdout, "Managed objects: %lu\n", (unsigned long)count);                                   /* Show heap cardinality without invoking object getters. */
    if (!debugger_ok(fvm_debug_native_function_count(context, &count), "debug_native_function_count")) /* Count the global native-function registry. */
    {                                                                                                  /* Begin conditional branch: Count the global native-function registry. */
        goto cleanup;                                                                                  /* Avoid indexing a registry whose count query failed. */
    } /* End conditional branch: Count the global native-function registry. */
    if (count)                                                                                                                 /* An empty native registry has no row zero. */
    {                                                                                                                          /* Inspect one complete borrowed native function name. */
        if (!debugger_ok(fvm_debug_native_function_at_bytes(context, 0, &name, &name_size), "debug_native_function_at_bytes")) /* Read the first counted registry key. */
        {                                                                                                                      /* Begin conditional branch: Read the first counted registry key. */
            goto cleanup;                                                                                                      /* Do not use a name from an unsuccessful query. */
        } /* End conditional branch: Read the first counted registry key. */
        fwrite(name, 1, name_size, stdout); /* Display the entire borrowed byte name. */
        fputc('\n', stdout);                /* End the name's console output before querying other registries. */
    } /* Finish optional native-name inspection. */
    if (!debugger_ok(fvm_debug_script_function_count(context, &count), "debug_script_function_count")) /* Count named script registry entries separately from native callbacks. */
    {                                                                                                  /* Begin conditional branch: Count named script registry entries separately from native callbacks. */
        goto cleanup;                                                                                  /* Keep a failed registry query visible. */
    } /* End conditional branch: Count named script registry entries separately from native callbacks. */
    fprintf(stdout, "Named script registry entries: %lu\n", (unsigned long)count); /* This empty-movie session may legitimately report zero. */
    if (!debugger_ok(fvm_mutex_create(context, &mutex), "mutex_create"))           /* Acquire one process-local shared-resource handle. */
    {                                                                              /* Begin conditional branch: Acquire one process-local shared-resource handle. */
        goto cleanup;                                                              /* Leave mutex ownership empty on failure. */
    } /* End conditional branch: Acquire one process-local shared-resource handle. */
    if (!debugger_ok(fvm_mutex_set(context, mutex, 0), "mutex_set")) /* Acquire without blocking, so the tutorial cannot wait in a deadlock. */
    {                                                                /* Begin conditional branch: Acquire without blocking, so the tutorial cannot wait in a deadlock. */
        goto cleanup;                                                /* Do not mark ownership after an unsuccessful acquisition. */
    } /* End conditional branch: Acquire without blocking, so the tutorial cannot wait in a deadlock. */
    locked = 1;                                                   /* Remember that this context must release its ownership later. */
    status = fvm_debug_mutex_detail_snapshot(0, 0, &needed);      /* Measure the coherent process-local registry snapshot size. */
    if (status != FVM_OK && status != FVM_NOT_FOUND)              /* NOT_FOUND means the caller must provide the reported capacity. */
    {                                                             /* Preserve an unexpected measurement failure explicitly. */
        debugger_ok(status, "debug_mutex_detail_snapshot probe"); /* Print the actual probe result. */
        goto cleanup;                                             /* Avoid allocating from an unreliable capacity measurement. */
    } /* Finish checking the snapshot capacity probe. */
    if (needed > (size_t)-1 / sizeof(*rows)) /* Guard the host array-size multiplication before allocating. */
    {                                        /* Begin conditional branch: Guard the host array-size multiplication before allocating. */
        goto cleanup;                        /* Keep an unrepresentable snapshot from overflowing allocation size. */
    } /* End conditional branch: Guard the host array-size multiplication before allocating. */
    rows = (FVM_DebugMutexDetail *)malloc(needed * sizeof(*rows)); /* Allocate caller-owned output rows, not live registry handles. */
    if (!rows && needed)                                           /* A nonempty snapshot requires a successful host allocation. */
    {                                                              /* Begin conditional branch: A nonempty snapshot requires a successful host allocation. */
        goto cleanup;                                              /* Release mutex ownership before returning a host allocation failure. */
    } /* End conditional branch: A nonempty snapshot requires a successful host allocation. */
    if (!debugger_ok(fvm_debug_mutex_detail_snapshot(rows, needed, &count), "debug_mutex_detail_snapshot")) /* Copy exact rows under the registry lock, without remote RPC. */
    {                                                                                                       /* Begin conditional branch: Copy exact rows under the registry lock, without remote RPC. */
        goto cleanup;                                                                                       /* A concurrent capacity increase is reported instead of truncating rows. */
    } /* End conditional branch: Copy exact rows under the registry lock, without remote RPC. */
    for (index = 0; index < count; ++index)                                                                                                                                                                               /* Inspect copied rows without holding the registry lock. */
    {                                                                                                                                                                                                                     /* Begin loop body: Inspect copied rows without holding the registry lock. */
        fprintf(stdout, "Mutex %lu owner=%lu waiter=%lu local_deadlock=%d remote=%d\n", rows[index].state.identity, rows[index].state.owner, rows[index].state.waiter, rows[index].state.deadlocked, rows[index].remote); /* Keep remote observations distinct from local deadlock metadata. */
    } /* End loop body: Inspect copied rows without holding the registry lock. */
    if (!debugger_ok(fvm_mutex_reset(context, mutex), "mutex_reset")) /* Release ownership through the actual owner's context. */
    {                                                                 /* Begin conditional branch: Release ownership through the actual owner's context. */
        goto cleanup;                                                 /* Preserve the locked flag if release fails. */
    } /* End conditional branch: Release ownership through the actual owner's context. */
    locked = 0;                          /* Prevent cleanup from resetting an already-released mutex. */
    exit_code = 0;                       /* Both registry and resource inspection completed successfully. */
cleanup:                                 /* Join ownership paths without hiding failed inspection operations. */
    free(rows);                          /* Release only the caller-owned copied row storage. */
    if (locked)                          /* A setup or snapshot error may have left ownership active. */
    {                                    /* Begin conditional branch: A setup or snapshot error may have left ownership active. */
        fvm_mutex_reset(context, mutex); /* Attempt owner-only cleanup before releasing the handle. */
    } /* End conditional branch: A setup or snapshot error may have left ownership active. */
    if (mutex)                    /* Creation may have failed before a resource handle existed. */
    {                             /* Begin conditional branch: Creation may have failed before a resource handle existed. */
        fvm_mutex_release(mutex); /* Release the native handle after its owner reset attempt. */
    } /* End conditional branch: Creation may have failed before a resource handle existed. */
    debugger_close(runtime, context); /* Drop registry views and runtime ownership last. */
    return exit_code;                 /* Return the ordinary checked host outcome. */
} /* End the coherent resource snapshot tutorial. */
