/* C89 cursor-request tutorial: native window/device ownership remains outside the VM. */
#include "flashvm/mouse.h"    /* Query requested visibility and atomically obtain previous state. */
#include "flashvm/standard.h" /* Install standard objects before the explicit Mouse namespace. */
#include "flashvm/value.h"    /* Retrieve Mouse and call its native script-visible methods. */
#include <stdio.h>            /* Report ordinary status/output without assertions. */


/* [GC:] No managed object escapes; runtime/context are the only VM owners. */
int main(void)                                          /* All state transitions stay on one owning thread. */
{                                                       /* Initialize partial ownership before any operation can fail. */
    FVM_Config config;                                  /* Caller-owned configuration storage. */
    FVM_Runtime *runtime = 0;                           /* Destroy only after context. */
    FVM_Context *context = 0;                           /* Owns its independent requested cursor state. */
    FVM_Value mouse, out;                               /* [GC:] Namespace remains rooted by the global binding; results are Numbers. */
    int script_hidden = -1, script_shown = -1;          /* Previous states returned by script-visible methods. */
    int visible = -1;                                   /* Writable scalar getter output. */
    int first = -1, repeated = -1, restored = -1;       /* Previous requested states from three transitions. */
    FVM_Result status = fvm_config_init(&config);       /* Initialize ABI defaults before runtime acquisition. */
    if (status == FVM_OK)                               /* Do not acquire owners after an earlier failure. */
    {                                                   /* Begin conditional branch: Do not acquire owners after an earlier failure. */
        status = fvm_runtime_create(&config, &runtime); /* Acquire outer VM owner. */
    } /* End conditional branch: Do not acquire owners after an earlier failure. */
    if (status == FVM_OK)                               /* Keep each native window's logical state in its own context. */
    {                                                   /* Begin conditional branch: Keep each native window's logical state in its own context. */
        status = fvm_context_create(runtime, &context); /* Default request is visible. */
    } /* End conditional branch: Keep each native window's logical state in its own context. */
    if (status == FVM_OK)                       /* Prepare ordinary Object support before the explicit input namespace. */
    {                                           /* Begin conditional branch: Prepare ordinary Object support before the explicit input namespace. */
        status = fvm_standard_install(context); /* No physical window or cursor is acquired. */
    } /* End conditional branch: Prepare ordinary Object support before the explicit input namespace. */
    if (status == FVM_OK)                             /* Install Mouse.hide/show and version-gated listener methods atomically. */
    {                                                 /* Begin conditional branch: Install Mouse.hide/show and version-gated listener methods atomically. */
        status = fvm_standard_mouse_install(context); /* Host context supports the complete installed subset. */
    } /* End conditional branch: Install Mouse.hide/show and version-gated listener methods atomically. */
    if (status == FVM_OK)                                         /* Read logical request, not physical OS cursor state. */
    {                                                             /* Begin conditional branch: Read logical request, not physical OS cursor state. */
        status = fvm_input_get_cursor_visible(context, &visible); /* No allocation, script or device query. */
    } /* End conditional branch: Read logical request, not physical OS cursor state. */
    if (status == FVM_OK && visible != 1) /* Validate the documented initial request. */
    {                                     /* Begin conditional branch: Validate the documented initial request. */
        status = FVM_ERROR;               /* Ordinary tutorial error path. */
    } /* End conditional branch: Validate the documented initial request. */
    if (status == FVM_OK)                                          /* Request hidden, preserving the previous request in first. */
    {                                                              /* Begin conditional branch: Request hidden, preserving the previous request in first. */
        status = fvm_input_set_cursor_visible(context, 0, &first); /* Expected previous=1. */
    } /* End conditional branch: Request hidden, preserving the previous request in first. */
    if (status == FVM_OK)                                             /* Repeated hide must still report the previous logical state. */
    {                                                                 /* Begin conditional branch: Repeated hide must still report the previous logical state. */
        status = fvm_input_set_cursor_visible(context, 0, &repeated); /* Expected previous=0. */
    } /* End conditional branch: Repeated hide must still report the previous logical state. */
    if (status == FVM_OK)                                             /* Restore visibility without changing input, focus or subscriptions. */
    {                                                                 /* Begin conditional branch: Restore visibility without changing input, focus or subscriptions. */
        status = fvm_input_set_cursor_visible(context, 1, &restored); /* Expected previous=0. */
    } /* End conditional branch: Restore visibility without changing input, focus or subscriptions. */
    if (status == FVM_OK)                                         /* A window host would apply this result through its own SDK. */
    {                                                             /* Begin conditional branch: A window host would apply this result through its own SDK. */
        status = fvm_input_get_cursor_visible(context, &visible); /* Expected requested visibility=1. */
    } /* End conditional branch: A window host would apply this result through its own SDK. */
    if (status == FVM_OK && (first != 1 || repeated != 0 || restored != 0 || visible != 1)) /* Check all outputs. */
    {                                                                                       /* Begin conditional branch: Check all outputs. */
        status = FVM_ERROR;                                                                 /* Do not claim success after an unexpected state transition. */
    } /* End conditional branch: Check all outputs. */
    if (status == FVM_OK)                                  /* Borrow the globally rooted namespace for two synchronous calls. */
    {                                                      /* Begin conditional branch: Borrow the globally rooted namespace for two synchronous calls. */
        status = fvm_get_global(context, "Mouse", &mouse); /* [GC:] No managed pointer escapes this host. */
    } /* End conditional branch: Borrow the globally rooted namespace for two synchronous calls. */
    if (status == FVM_OK)                                                    /* Invoke the same method available to an AS1/AS2 movie. */
    {                                                                        /* Begin conditional branch: Invoke the same method available to an AS1/AS2 movie. */
        status = fvm_object_call(context, mouse.object, "hide", 0, 0, &out); /* No arguments or coercion. */
    } /* End conditional branch: Invoke the same method available to an AS1/AS2 movie. */
    if (status == FVM_OK && out.type != FVM_VALUE_NUMBER) /* Condition for this action: Validate public return type. */
    {                                                     /* Begin conditional branch: Condition for this action: Validate public return type. */
        status = FVM_ERROR;                               /* Validate public return type. */
    } /* End conditional branch: Condition for this action: Validate public return type. */
    if (status == FVM_OK)                                           /* Condition for this action: Copy the previous requested visibility. */
    {                                                               /* Begin conditional branch: Condition for this action: Copy the previous requested visibility. */
        script_hidden = out.number == 1 ? 1 : out.number == 0 ? 0   /* Copy the previous requested visibility. */
                                                              : -1; /* Copy the previous requested visibility. */
    } /* End conditional branch: Condition for this action: Copy the previous requested visibility. */
    if (status == FVM_OK)                                                    /* Restore through the script-visible method, not a native cursor SDK. */
    {                                                                        /* Begin conditional branch: Restore through the script-visible method, not a native cursor SDK. */
        status = fvm_object_call(context, mouse.object, "show", 0, 0, &out); /* Expect old request zero. */
    } /* End conditional branch: Restore through the script-visible method, not a native cursor SDK. */
    if (status == FVM_OK && out.type != FVM_VALUE_NUMBER) /* Condition for this action: Do not inspect an invalid result. */
    {                                                     /* Begin conditional branch: Condition for this action: Do not inspect an invalid result. */
        status = FVM_ERROR;                               /* Do not inspect an invalid result. */
    } /* End conditional branch: Condition for this action: Do not inspect an invalid result. */
    if (status == FVM_OK)                                          /* Condition for this action: Retain scalar after the next VM call. */
    {                                                              /* Begin conditional branch: Condition for this action: Retain scalar after the next VM call. */
        script_shown = out.number == 1 ? 1 : out.number == 0 ? 0   /* Retain scalar after the next VM call. */
                                                             : -1; /* Retain scalar after the next VM call. */
    } /* End conditional branch: Condition for this action: Retain scalar after the next VM call. */
    if (status == FVM_OK && (script_hidden != 1 || script_shown != 0)) /* Condition for this action: Check semantics. */
    {                                                                  /* Begin conditional branch: Condition for this action: Check semantics. */
        status = FVM_ERROR;                                            /* Check semantics. */
    } /* End conditional branch: Condition for this action: Check semantics. */
    if (status == FVM_OK)                                                               /* No physical cursor is needed for this deterministic example. */
    {                                                                                   /* Begin conditional branch: No physical cursor is needed for this deterministic example. */
        printf("previous=%d,%d,%d requested=%d\n", first, repeated, restored, visible); /* Expect 1,0,0 and 1. */
    } /* End conditional branch: No physical cursor is needed for this deterministic example. */
    if (status == FVM_OK)                                               /* Condition for this action: Expect 1,0. */
    {                                                                   /* Begin conditional branch: Condition for this action: Expect 1,0. */
        printf("script previous=%d,%d\n", script_hidden, script_shown); /* Expect 1,0. */
    } /* End conditional branch: Condition for this action: Expect 1,0. */
    else                                                             /* Preserve the first VM failure. */
    {                                                                /* Begin conditional branch: Preserve the first VM failure. */
        fprintf(stderr, "cursor request failed: %d\n", (int)status); /* Visible nonzero exit follows. */
    } /* End conditional branch: Preserve the first VM failure. */
    if (context)                      /* Condition for this action: [GC:] No callback or object borrow survives cleanup. */
    {                                 /* Begin conditional branch: Condition for this action: [GC:] No callback or object borrow survives cleanup. */
        fvm_context_destroy(context); /* [GC:] No callback or object borrow survives cleanup. */
    } /* End conditional branch: Condition for this action: [GC:] No callback or object borrow survives cleanup. */
    if (runtime)                      /* Condition for this action: Release outer owner last, including partial failures. */
    {                                 /* Begin conditional branch: Condition for this action: Release outer owner last, including partial failures. */
        fvm_runtime_destroy(runtime); /* Release outer owner last, including partial failures. */
    } /* End conditional branch: Condition for this action: Release outer owner last, including partial failures. */
    return status == FVM_OK ? 0 : 1; /* Report actual API success/error to the shell. */
} /* Native windows/cursors remain the responsibility of the embedding application. */
