/* Retain a first-chance AVM1 backtrace and print its JSON through the public C89 API. */
#include "../support.h" /* Reuse checked runtime/context creation and explicit cleanup. */
#include "flashvm/crash.h" /* Declare fault policy, copied frames and retained report views. */
#include "flashvm/execution_mode.h" /* Allow this example to request automatic JIT with fallback. */
#include "flashvm/exception.h" /* Clear the deliberately thrown ActionScript value after inspection. */

/* Run one throwing function and keep the host process alive to inspect the report. */
int main(void) /* No command-line arguments are needed for this in-memory example. */
{ /* Begin the host program. */
    const unsigned char actions[] = {0x96, 3, 0, 0, 'x', 0, 0x1c, 0x2a, 0}; /* Encode Push("x"), GetVariable, Throw and End. */
    const char *parameters[] = {"x"}; /* Give the supplied argument a named AVM1 binding. */
    FVM_Runtime *runtime = 0; /* Own the runtime only after debugger_open succeeds. */
    FVM_Context *context = 0; /* Keep VM ownership separate from borrowed report views. */
    FVM_Object *function = 0; /* Borrow the managed function until context destruction. */
    FVM_Value argument, result; /* Store the input and ordinary invocation output on the host stack. */
    FVM_Result status; /* Preserve the original VM result separately from debugger API results. */
    const void *json = 0; /* Borrow the retained JSON only while the report remains unchanged. */
    size_t json_size = 0; /* Use the counted JSON length instead of assuming a terminating NUL. */
    int exit_code = 1; /* Report a host failure unless every expected operation succeeds. */

    if (!debugger_open(&runtime, &context)) /* Initialize the runtime and standard classes. */
        goto cleanup; /* Release any partial setup ownership. */
    if (!debugger_ok(fvm_crash_configure(context, FVM_CRASH_BACKTRACE, 0, 0, 0), "crash_configure")) /* Retain a trace without writing a file. */
        goto cleanup; /* Preserve the printed configuration error. */
    if (!debugger_ok(fvm_execution_set_mode(context, FVM_EXECUTION_JIT, 0), "execution_set_mode")) /* Use native compilation where supported and interpreter fallback elsewhere. */
        goto cleanup; /* Stop before executing under an unintended policy. */
    if (!debugger_ok(fvm_function_create_avm1(context, actions, sizeof(actions), 1, parameters, 0, &function), "function_create")) /* Create one ordinary AVM1 activation. */
        goto cleanup; /* Keep the runtime alive only long enough to clean it up. */
    fvm_value_number(&argument, 42); /* Supply a numeric argument that will be visible in the trace. */
    status = fvm_function_invoke(context, function, 0, 1, &argument, &result); /* Let the VM return the deliberate script exception. */
    if (status != FVM_EXCEPTION) /* The tutorial expects a script throw rather than successful completion. */
    { /* Report an unexpected execution result without a process assertion. */
        fprintf(stderr, "Expected a script exception, received %d\n", (int)status); /* Include the actual return code. */
        goto cleanup; /* Release the VM after the unexpected outcome. */
    } /* Finish validation of the deliberately failing call. */
    if (!debugger_ok(fvm_crash_data(context, 0, &json, &json_size), "crash_data")) /* Borrow the copied trace after the call has unwound. */
        goto cleanup; /* Avoid reading an absent or failed report. */
    if (fwrite(json, 1, json_size, stdout) != json_size) /* Write exactly the retained JSON bytes. */
        goto cleanup; /* Propagate a host output failure through the exit status. */
    fputc('\n', stdout); /* Separate the JSON document from the shell prompt. */
    if (!debugger_ok(fvm_exception_clear(context), "exception_clear")) /* Finish handling the deliberately pending throw. */
        goto cleanup; /* Keep a failed clear visible to the host. */
    exit_code = 0; /* The expected VM failure was successfully inspected. */

cleanup: /* Join successful and unsuccessful paths at one ownership boundary. */
    debugger_close(runtime, context); /* Invalidate borrowed report views and release the isolated VM. */
    return exit_code; /* Return a checked tutorial result to the shell. */
} /* End the retained-backtrace example. */
