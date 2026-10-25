/* Keep native source/machine association and produce a postmortem ZIP through public APIs. */
#include "../support.h"             /* Share checked VM setup and ownership cleanup. */
#include "flashvm/crash.h"          /* Declare ZIP_CODE and copied frame inspection. */
#include "flashvm/jit.h"            /* Declare explicit native compilation and immutable mapping views. */
#include "flashvm/execution_mode.h" /* Select manual attachment-compatible execution explicitly. */
#include "flashvm/exception.h"      /* Clear the deliberately thrown script value after inspection. */


/* Compile a throwing AVM1 body, dump it and release code only after its context is gone. */
int main(int argc, char **argv)                                            /* An optional argument selects the output ZIP path. */
{                                                                          /* Begin the native-dump host program. */
    const unsigned char actions[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x2a, 0}; /* Encode integer 42, Throw and End. */
    const char *path = argc > 1 ? argv[1] : "debugger-jit.zip";            /* Make output-file overwrite an explicit example choice. */
    FVM_Runtime *runtime = 0;                                              /* Own a runtime independently of the host-owned native code. */
    FVM_Context *context = 0;                                              /* Own the function and retained diagnostic report in this VM. */
    FVM_Object *function = 0;                                              /* Borrow the script function until context destruction. */
    FVM_JitCode *code = 0;                                                 /* Own the explicit native handle after successful compilation. */
    FVM_Value result;                                                      /* Receive ordinary invocation output separately from the thrown value. */
    FVM_Result diagnostic = FVM_OK, original = FVM_OK;                     /* Keep capture/file errors separate from execution status. */
    size_t frames = 0;                                                     /* Receive the number of copied postmortem frames. */
    FVM_Result status;                                                     /* Retain the original script execution result. */
    const void *view = 0;                                                  /* Borrow either immutable code bytes or retained ZIP bytes. */
    size_t size = 0;                                                       /* Carry the exact byte count of the current borrowed view. */
    int exit_code = 1;                                                     /* Return failure unless the deliberate crash report is produced correctly. */
    if (!debugger_open(&runtime, &context))                                /* Create a VM and its standard classes. */
    {                                                                      /* Begin conditional branch: Create a VM and its standard classes. */
        goto cleanup;                                                      /* Keep partial ownership visible to the common cleanup path. */
    } /* End conditional branch: Create a VM and its standard classes. */
    if (!debugger_ok(fvm_crash_configure(context, FVM_CRASH_ZIP_CODE, path, 0, 0), "crash_configure")) /* Request raw state plus the active emitted instruction mapping. */
    {                                                                                                  /* Begin conditional branch: Request raw state plus the active emitted instruction mapping. */
        goto cleanup;                                                                                  /* Avoid execution if the output policy could not be installed. */
    } /* End conditional branch: Request raw state plus the active emitted instruction mapping. */
    if (!debugger_ok(fvm_execution_set_mode(context, FVM_EXECUTION_MANUAL, 0), "execution_set_mode")) /* Honor an explicit body attachment without automatic replacement. */
    {                                                                                                 /* Begin conditional branch: Honor an explicit body attachment without automatic replacement. */
        goto cleanup;                                                                                 /* Keep interpretation/JIT policy independent of dump configuration. */
    } /* End conditional branch: Honor an explicit body attachment without automatic replacement. */
    if (!debugger_ok(fvm_function_create_avm1(context, actions, sizeof(actions), 0, 0, 0, &function), "function_create")) /* Preserve the original interpreted body in the function. */
    {                                                                                                                     /* Begin conditional branch: Preserve the original interpreted body in the function. */
        goto cleanup;                                                                                                     /* Release the VM if function creation fails. */
    } /* End conditional branch: Preserve the original interpreted body in the function. */
    if (fvm_jit_available())                                                               /* Native mapping support is an actual host/ABI capability. */
    {                                                                                      /* Compile and inspect real native code when this host supports it. */
        if (!debugger_ok(fvm_jit_compile(actions, sizeof(actions), &code), "jit_compile")) /* Acquire a host-owned immutable source/machine-code pair. */
        {                                                                                  /* Begin conditional branch: Acquire a host-owned immutable source/machine-code pair. */
            goto cleanup;                                                                  /* Keep the code pointer null if compilation failed. */
        } /* End conditional branch: Acquire a host-owned immutable source/machine-code pair. */
        if (!debugger_ok(fvm_jit_source(code, &view, &size), "jit_source")) /* Obtain the AVM1 snapshot tied to this native handle. */
        {                                                                   /* Begin conditional branch: Obtain the AVM1 snapshot tied to this native handle. */
            goto cleanup;                                                   /* Do not use a failed borrowed source view. */
        } /* End conditional branch: Obtain the AVM1 snapshot tied to this native handle. */
        fprintf(stdout, "AVM1 source: %lu bytes\n", (unsigned long)size);               /* Display source size without decoding or modifying it. */
        if (!debugger_ok(fvm_jit_machine_code(code, &view, &size), "jit_machine_code")) /* Obtain the read-only RX mapping payload. */
        {                                                                               /* Begin conditional branch: Obtain the read-only RX mapping payload. */
            goto cleanup;                                                               /* Leave the mapping unmodified on inspection failure. */
        } /* End conditional branch: Obtain the read-only RX mapping payload. */
        fprintf(stdout, "Native mapping: %lu bytes, backend %s\n", (unsigned long)size, fvm_jit_backend_name()); /* Identify the actual generated ABI. */
        if (!debugger_ok(fvm_function_set_body(context, function, fvm_jit_body, code, 0), "function_set_body"))  /* Attach the code while its host ownership remains alive. */
        {                                                                                                        /* Begin conditional branch: Attach the code while its host ownership remains alive. */
            goto cleanup;                                                                                        /* Destroy the context before releasing any possibly attached code. */
        } /* End conditional branch: Attach the code while its host ownership remains alive. */
    } /* End the host-specific native path. */
    else                                                                                                  /* Unsupported hosts can still demonstrate source/state ZIP capture. */
    {                                                                                                     /* Begin conditional branch: Unsupported hosts can still demonstrate source/state ZIP capture. */
        fprintf(stdout, "Native backend unavailable; the dump will contain interpreted source/state.\n"); /* Describe the expected absence of emitted bytes. */
    } /* End conditional branch: Unsupported hosts can still demonstrate source/state ZIP capture. */
    status = fvm_function_invoke(context, function, 0, 0, 0, &result);              /* Execute the deliberate Throw through the ordinary invocation ABI. */
    if (status != FVM_EXCEPTION)                                                    /* A completed or differently failed call is not this tutorial's intended result. */
    {                                                                               /* Report an unexpected execution outcome. */
        fprintf(stderr, "Expected a script exception, received %d\n", (int)status); /* Preserve the actual VM status. */
        goto cleanup;                                                               /* Keep native ownership alive until context shutdown. */
    } /* Finish checking the deliberate failure. */
    if (!debugger_ok(fvm_crash_info(context, &frames, &original, &diagnostic), "crash_info")) /* Check that a new copied report actually exists. */
    {                                                                                         /* Begin conditional branch: Check that a new copied report actually exists. */
        goto cleanup;                                                                         /* Do not treat an older file as the current fault report. */
    } /* End conditional branch: Check that a new copied report actually exists. */
    if (!debugger_ok(diagnostic, "fault report capture/write")) /* Capture success is independent of the expected script exception. */
    {                                                           /* Begin conditional branch: Capture success is independent of the expected script exception. */
        goto cleanup;                                           /* Report file-system or allocation failures instead of claiming a written ZIP. */
    } /* End conditional branch: Capture success is independent of the expected script exception. */
    if (!debugger_ok(fvm_crash_data(context, 1, &view, &size), "crash_data")) /* Read the ZIP retained in memory as well as written to path. */
    {                                                                         /* Begin conditional branch: Read the ZIP retained in memory as well as written to path. */
        goto cleanup;                                                         /* Avoid reporting a nonexistent archive. */
    } /* End conditional branch: Read the ZIP retained in memory as well as written to path. */
    fprintf(stdout, "Fault ZIP: %s (%lu bytes)\n", path, (unsigned long)size); /* Tell the host where the inspectable artifact was written. */
    if (!debugger_ok(fvm_exception_clear(context), "exception_clear"))         /* Finish handling the deliberately pending script throw. */
    {                                                                          /* Begin conditional branch: Finish handling the deliberately pending script throw. */
        goto cleanup;                                                          /* Keep the host result unsuccessful if pending state cannot be cleared. */
    } /* End conditional branch: Finish handling the deliberately pending script throw. */
    exit_code = 0;                                                  /* The deliberate VM failure produced an inspectable ZIP. */
cleanup:                                                            /* Close the owning context before releasing a potentially attached native body. */
    debugger_close(runtime, context);                               /* Drop all managed function attachments and report views first. */
    if (code && !debugger_ok(fvm_jit_destroy(code), "jit_destroy")) /* Release the explicit host-owned mapping after every attachment is gone. */
    {                                                               /* Begin conditional branch: Release the explicit host-owned mapping after every attachment is gone. */
        exit_code = 1;                                              /* Surface an unexpected native-code lifetime error. */
    } /* End conditional branch: Release the explicit host-owned mapping after every attachment is gone. */
    return exit_code; /* Preserve the checked result of the demonstration. */
} /* End the source/native ZIP example. */
