/* C89 cursor-request tutorial: native window/device ownership remains outside the VM. */
#include "flashvm/mouse.h" /* Query requested visibility and atomically obtain previous state. */
#include <stdio.h> /* Report ordinary status/output without assertions. */
/* [GC:] No managed object escapes; runtime/context are the only VM owners. */
int main(void) /* All state transitions stay on one owning thread. */
{ /* Initialize partial ownership before any operation can fail. */
    FVM_Config config; /* Caller-owned configuration storage. */
    FVM_Runtime *runtime = 0; /* Destroy only after context. */
    FVM_Context *context = 0; /* Owns its independent requested cursor state. */
    int visible = -1; /* Writable scalar getter output. */
    int first = -1, repeated = -1, restored = -1; /* Previous requested states from three transitions. */
    FVM_Result status = fvm_config_init(&config); /* Initialize ABI defaults before runtime acquisition. */
    if (status == FVM_OK) /* Do not acquire owners after an earlier failure. */
        status = fvm_runtime_create(&config, &runtime); /* Acquire outer VM owner. */
    if (status == FVM_OK) /* Keep each native window's logical state in its own context. */
        status = fvm_context_create(runtime, &context); /* Default request is visible. */
    if (status == FVM_OK) /* Read logical request, not physical OS cursor state. */
        status = fvm_input_get_cursor_visible(context, &visible); /* No allocation, script or device query. */
    if (status == FVM_OK && visible != 1) /* Validate the documented initial request. */
        status = FVM_ERROR; /* Ordinary tutorial error path. */
    if (status == FVM_OK) /* Request hidden, preserving the previous request in first. */
        status = fvm_input_set_cursor_visible(context, 0, &first); /* Expected previous=1. */
    if (status == FVM_OK) /* Repeated hide must still report the previous logical state. */
        status = fvm_input_set_cursor_visible(context, 0, &repeated); /* Expected previous=0. */
    if (status == FVM_OK) /* Restore visibility without changing input, focus or subscriptions. */
        status = fvm_input_set_cursor_visible(context, 1, &restored); /* Expected previous=0. */
    if (status == FVM_OK) /* A window host would apply this result through its own SDK. */
        status = fvm_input_get_cursor_visible(context, &visible); /* Expected requested visibility=1. */
    if (status == FVM_OK && (first != 1 || repeated != 0 || restored != 0 || visible != 1)) /* Check all outputs. */
        status = FVM_ERROR; /* Do not claim success after an unexpected state transition. */
    if (status == FVM_OK) /* No physical cursor is needed for this deterministic example. */
        printf("previous=%d,%d,%d requested=%d\n", first, repeated, restored, visible); /* Expect 1,0,0 and 1. */
    else /* Preserve the first VM failure. */
        fprintf(stderr, "cursor request failed: %d\n", (int)status); /* Visible nonzero exit follows. */
    if (context) fvm_context_destroy(context); /* [GC:] No callback or object borrow survives cleanup. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release outer owner last, including partial failures. */
    return status == FVM_OK ? 0 : 1; /* Report actual API success/error to the shell. */
} /* Native windows/cursors remain the responsibility of the embedding application. */
