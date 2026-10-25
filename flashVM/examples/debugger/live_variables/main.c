/* Pause before unwinding and inspect original arguments, materialized locals and registers. */
#include "../support.h"             /* Provide explicit setup/error handling without process assertions. */
#include "flashvm/debug.h"          /* Declare synchronous fault inspection and preservation policies. */
#include "flashvm/execution_mode.h" /* Request genuine automatic JIT execution where available. */
#include "flashvm/exception.h"      /* Handle the script exception after the debugger returns. */


/* Inspect borrowed activation values while the debugger callback keeps the VM paused. */
static void inspect(FVM_Context *context, FVM_Result failure, void *userdata)   /* Borrow the VM and the host-owned callback status slot. */
{                                                                               /* Begin the synchronous debugger stop; no script reentry or GC is performed. */
    int *callback_ok = (int *)userdata;                                         /* Return inspection success to main without changing VM status. */
    FVM_DebugFrame frame;                                                       /* Check whether requested entry preservation actually succeeded. */
    FVM_Value value;                                                            /* Receive one borrowed argument/local/register value at a time. */
    const char *name = 0;                                                       /* Borrow a local name only until this callback returns. */
    size_t name_size = 0;                                                       /* Preserve counted names, including any embedded NUL bytes. */
    size_t index = 0;                                                           /* Enumerate available activation fields in the VM's raw storage order. */
    FVM_Result status;                                                          /* Distinguish end-of-enumeration from an API failure. */
    fprintf(stdout, "Debugger stopped for result %d\n", (int)failure);          /* Explain why the host callback was entered. */
    if (!debugger_ok(fvm_debug_frame_at(context, 0, &frame), "debug_frame_at")) /* Read the innermost borrowed live frame. */
    {                                                                           /* Begin conditional branch: Read the innermost borrowed live frame. */
        return;                                                                 /* Keep inspection unsuccessful if no live frame exists. */
    } /* End conditional branch: Read the innermost borrowed live frame. */
    if (!frame.forced || frame.preservation_status != FVM_OK) /* Distinguish failed FORCE allocation from successful preservation. */
    {                                                         /* Begin conditional branch: Distinguish failed FORCE allocation from successful preservation. */
        return;                                               /* Do not claim original entry values survived a preservation failure. */
    } /* End conditional branch: Distinguish failed FORCE allocation from successful preservation. */
    if (!debugger_ok(fvm_crash_argument(context, 0, 0, &value), "crash_argument")) /* Read the innermost function's original supplied argument. */
    {                                                                              /* Begin conditional branch: Read the innermost function's original supplied argument. */
        return;                                                                    /* Leave callback_ok false if the live argument is unavailable. */
    } /* End conditional branch: Read the innermost function's original supplied argument. */
    fprintf(stdout, "Original argument: %.17g\n", value.number);                /* Print the numeric value used by this particular fixture. */
    for (;;)                                                                    /* Inspect locals only while the live activation is suspended. */
    {                                                                           /* Begin one raw-local query. */
        status = fvm_crash_local(context, 0, index, &name, &name_size, &value); /* Avoid getters, coercions and script callbacks. */
        if (status == FVM_NOT_FOUND)                                            /* An absent next row ends this materialized-local view. */
        {                                                                       /* Begin conditional branch: An absent next row ends this materialized-local view. */
            break;                                                              /* Do not invent values for unavailable or unnamed optimized state. */
        } /* End conditional branch: An absent next row ends this materialized-local view. */
        if (!debugger_ok(status, "crash_local")) /* Preserve unexpected query errors separately from the VM failure. */
        {                                        /* Begin conditional branch: Preserve unexpected query errors separately from the VM failure. */
            return;                              /* Let normal fault propagation continue after the debugger callback. */
        } /* End conditional branch: Preserve unexpected query errors separately from the VM failure. */
        fwrite(name, 1, name_size, stdout);                      /* Display the full counted local name. */
        fprintf(stdout, " has value tag %d\n", (int)value.type); /* Show type identity without invoking user formatting code. */
        ++index;                                                 /* Move to the next already-materialized local row. */
    } /* Finish the available-local enumeration. */
    status = fvm_crash_register(context, 0, 0, &value);                 /* Probe the existing ABI register bank without forcing an extra spill. */
    if (status == FVM_OK)                                               /* Not every activation has a register bank. */
    {                                                                   /* Begin conditional branch: Not every activation has a register bank. */
        fprintf(stdout, "Register zero has tag %d\n", (int)value.type); /* Report the available register's raw type. */
    } /* End conditional branch: Not every activation has a register bank. */
    else if (status != FVM_NOT_FOUND) /* Missing state is ordinary; other statuses are errors. */
    {                                 /* Begin conditional branch: Missing state is ordinary; other statuses are errors. */
        return;                       /* Keep callback_ok false for an unexpected register query failure. */
    } /* End conditional branch: Missing state is ordinary; other statuses are errors. */
    *callback_ok = 1; /* Tell main that all required live inspection succeeded. */
} /* Returning permits ordinary VM error unwinding; it does not retry the Throw instruction. */


/* Select explicit preservation while keeping interpreter/JIT execution policy independent. */
int main(void)                                                              /* Run entirely in memory using one deliberately throwing function. */
{                                                                           /* Begin the host program. */
    const unsigned char actions[] = {0x96, 3, 0, 0, 'x', 0, 0x1c, 0x2a, 0}; /* Throw the function's named x argument. */
    const char *parameters[] = {"x"};                                       /* Make x an ordinary inspectable activation binding. */
    FVM_Runtime *runtime = 0;                                               /* Track runtime ownership explicitly. */
    FVM_Context *context = 0;                                               /* Track the owning VM thread's context explicitly. */
    FVM_Object *function = 0;                                               /* Borrow the created managed function. */
    FVM_Value argument, result;                                             /* Hold the input and ordinary output cells. */
    FVM_Result status;                                                      /* Preserve the invocation result for explicit expected-error handling. */
    int callback_ok = 0;                                                    /* Keep host debugger success separate from VM execution success. */
    int exit_code = 1;                                                      /* Leave failure visible unless setup, stop and cleanup all succeed. */
    if (!debugger_open(&runtime, &context))                                 /* Create a VM containing the standard classes. */
    {                                                                       /* Begin conditional branch: Create a VM containing the standard classes. */
        goto cleanup;                                                       /* Release partial initialization on failure. */
    } /* End conditional branch: Create a VM containing the standard classes. */
    if (!debugger_ok(fvm_crash_set_detail(context, FVM_CRASH_DETAIL_FORCE), "crash_set_detail")) /* Retain supplied argument values and string bytes at entry. */
    {                                                                                            /* Begin conditional branch: Retain supplied argument values and string bytes at entry. */
        goto cleanup;                                                                            /* Avoid silently running under the cheaper preservation policy. */
    } /* End conditional branch: Retain supplied argument values and string bytes at entry. */
    if (!debugger_ok(fvm_crash_configure(context, FVM_CRASH_DEBUGGER, 0, inspect, &callback_ok), "crash_configure")) /* Install a host-owned synchronous debugger callback. */
    {                                                                                                                /* Begin conditional branch: Install a host-owned synchronous debugger callback. */
        goto cleanup;                                                                                                /* Keep the callback state on main's stack until VM shutdown. */
    } /* End conditional branch: Install a host-owned synchronous debugger callback. */
    if (!debugger_ok(fvm_execution_set_mode(context, FVM_EXECUTION_JIT, 0), "execution_set_mode")) /* Keep native JIT enabled independently of forced debugging detail. */
    {                                                                                              /* Begin conditional branch: Keep native JIT enabled independently of forced debugging detail. */
        goto cleanup;                                                                              /* Abort before creating a function under an unintended execution policy. */
    } /* End conditional branch: Keep native JIT enabled independently of forced debugging detail. */
    if (!debugger_ok(fvm_function_create_avm1(context, actions, sizeof(actions), 1, parameters, 0, &function), "function_create")) /* Prepare a standard script function. */
    {                                                                                                                              /* Begin conditional branch: Prepare a standard script function. */
        goto cleanup;                                                                                                              /* Release the VM if function creation fails. */
    } /* End conditional branch: Prepare a standard script function. */
    fvm_value_number(&argument, 42);                                                                          /* Supply the original value retained by FORCE mode. */
    status = fvm_function_invoke(context, function, 0, 1, &argument, &result);                                /* Enter the debugger when this function throws. */
    if (status != FVM_EXCEPTION || !callback_ok)                                                              /* Both the expected throw and successful inspection are required. */
    {                                                                                                         /* Report a failed demonstration without terminating through an assertion. */
        fprintf(stderr, "Debugger demonstration failed: status=%d inspected=%d\n", (int)status, callback_ok); /* Keep both outcomes visible. */
        goto cleanup;                                                                                         /* Destroy the context after the synchronous callback has returned. */
    } /* Finish explicit validation of the callback demonstration. */
    if (!debugger_ok(fvm_exception_clear(context), "exception_clear")) /* Mark the deliberate throw as handled by the embedding application. */
    {                                                                  /* Begin conditional branch: Mark the deliberate throw as handled by the embedding application. */
        goto cleanup;                                                  /* Preserve a failed clear as a host error. */
    } /* End conditional branch: Mark the deliberate throw as handled by the embedding application. */
    exit_code = 0;                    /* The live debugger inspection completed successfully. */
cleanup:                              /* Use one cleanup path for every owned handle. */
    debugger_close(runtime, context); /* Destroy the VM only after its active frames have returned. */
    return exit_code;                 /* Return ordinary host success or failure. */
} /* End the live-variable debugger example. */
