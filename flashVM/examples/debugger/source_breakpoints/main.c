/* Display SWD source text at an ordinary interpreted breakpoint, even when JIT was requested. */
#include "../support.h" /* Share explicit runtime setup and ordinary error cleanup. */
#include "fixture.h" /* Supply a tiny matching SWF/SWD pair without implicit file-system reads. */
#include "flashvm/source_debug.h" /* Declare validated SWD loading and current source-line views. */
#include "flashvm/debug.h" /* Declare named-bytecode breakpoints and their host callback. */
#include "flashvm/execution_mode.h" /* Request JIT independently of breakpoint exclusion policy. */
#include "flashvm/exception.h" /* Clear the fixture's deliberate Throw after resuming the breakpoint. */

/* A host UI would render these same borrowed file/line/content views before returning. */
static FVM_Result show_line(FVM_Context *context, const FVM_DebugBreakpoint *point, void *userdata) /* Borrow a source position while the VM is stopped. */
{ /* Begin one synchronous breakpoint stop. */
    FVM_DebugSourcePosition position; /* Receive file identity, one-based line and borrowed line text. */
    int *hits = (int *)userdata; /* Keep the host callback counter outside managed VM state. */
    if (!debugger_ok(fvm_debug_current_source(context, &position), "debug_current_source")) /* Resolve the actual active body through its matching SWD bundle. */
        return FVM_ERROR; /* Stop the VM call if the requested source position is unavailable. */
    fwrite(position.source, 1, position.source_size, stdout); /* Display the author-provided filename without opening that path. */
    fprintf(stdout, ":%lu (AVM1 offset %lu)\n", position.line, point->offset); /* Identify the current line and exact action offset. */
    fwrite(position.line_text, 1, position.line_size, stdout); /* Render only the executing line's counted bytes. */
    fputc('\n', stdout); /* Finish the textual source marker for this console tutorial. */
    ++*hits; /* Confirm that execution entered the source-aware debugger stop. */
    return FVM_OK; /* Resume ordinary interpretation after inspecting the current source line. */
} /* The borrowed source pointers remain owned by the attached symbol bundle. */

/* Import source metadata, install a breakpoint and run the fixture's action body. */
int main(void) /* The embedded fixture keeps the tutorial deterministic and self-contained. */
{ /* Begin the host program. */
    FVM_Runtime *runtime = 0; /* Own a runtime for this independent debugging session. */
    FVM_Context *context = 0; /* Keep symbol and breakpoint ownership in one context. */
    FVM_Result status; /* Preserve the deliberate fixture exception after the breakpoint resumes. */
    int hits = 0; /* Count debugger entries without modifying script variables. */
    int exit_code = 1; /* Return success only after import, breakpoint and exception handling succeed. */
    if (!debugger_open(&runtime, &context)) /* Initialize the VM and standard classes. */
        goto cleanup; /* Release any successfully initialized handles on failure. */
    if (!debugger_ok(fvm_debug_load_swd(context, debugger_movie, sizeof(debugger_movie), debugger_symbols, sizeof(debugger_symbols)), "debug_load_swd")) /* Validate the shared DebugID before accepting file/line records. */
        goto cleanup; /* Never display source from an unrelated symbol bundle. */
    if (!debugger_ok(fvm_execution_set_mode(context, FVM_EXECUTION_JIT, 0), "execution_set_mode")) /* Demonstrate that breakpoint policy is independent of the requested JIT mode. */
        goto cleanup; /* Leave the VM unexecuted if mode configuration fails. */
    if (!debugger_ok(fvm_debug_set_breakpoint(context, "", 8, 1), "debug_set_breakpoint")) /* Stop at the root body's Throw action, eight bytes after its start. */
        goto cleanup; /* Keep a failed breakpoint installation visible. */
    if (!debugger_ok(fvm_debug_set_breakpoint_handler(context, show_line, &hits), "debug_set_breakpoint_handler")) /* Borrow main's hit counter until the synchronous call finishes. */
        goto cleanup; /* Avoid executing with a missing host debugger callback. */
    status = fvm_avm1_execute(context, debugger_actions, sizeof(debugger_actions)); /* Named-body breakpoint exclusion routes this entry through the interpreter. */
    if (status != FVM_EXCEPTION || hits != 1) /* The fixture must stop once and then execute its deliberate Throw. */
    { /* Report an unexpected breakpoint or execution result. */
        fprintf(stderr, "Source breakpoint result=%d hits=%d\n", (int)status, hits); /* Preserve both outcomes for the host. */
        goto cleanup; /* Release symbols only after the callback and execution have returned. */
    } /* Finish explicit validation of the source-aware stop. */
    if (!debugger_ok(fvm_exception_clear(context), "exception_clear")) /* Mark the expected script exception as handled. */
        goto cleanup; /* Keep failure visible if pending state cannot be cleared. */
    if (!debugger_ok(fvm_debug_clear_symbols(context), "debug_clear_symbols")) /* Release copied source metadata outside execution. */
        goto cleanup; /* Do not ignore a symbol-lifetime violation. */
    exit_code = 0; /* The source text and executing line were successfully displayed. */
cleanup: /* Join all ownership paths after synchronous execution. */
    debugger_close(runtime, context); /* Destroy breakpoints, callbacks and any remaining copied symbols. */
    return exit_code; /* Return an ordinary checked host result. */
} /* End the source/SWD breakpoint tutorial. */
