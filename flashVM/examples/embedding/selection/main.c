/* Compare default rejection, explicit MovieClip eligibility and Selection focus clearing. */
#include "flashvm/keyboard.h"  /* Declare the optional Selection installer. */
#include "flashvm/movieclip.h" /* Create a native MovieClip receiver. */
#include "flashvm/standard.h"  /* Install standard constructors and prototypes. */
#include "flashvm/gc.h"        /* Protect the clip before eligibility and focus are configured. */
#include <stdio.h>             /* Print ordinary results and errors without tutorial assertions. */


/* Report a synchronous Selection notification; SELECTION.md#focus-notifications documents arguments. */
static FVM_Result focus_event(FVM_Context *context, FVM_Object *receiver, unsigned int count, /* Borrow owners. */
                              const FVM_Value *args, FVM_Value *out, void *userdata)          /* Borrow one other clip. */
{                                                                                             /* Native callbacks do not own the receiver or argument. */
    (void)context;                                                                            /* This simple callback performs no nested VM operations. */
    (void)receiver;                                                                           /* Method dispatch already supplies the selected/previous receiver. */
    printf("%s other-null=%d\n", (const char *)userdata,                                      /* Show the event identity supplied at setup. */
           count == 1 && args[0].type == FVM_VALUE_NULL);                                     /* The tutorial transitions to/from no focus. */
    fvm_value_undefined(out);                                                                 /* Event return values are intentionally ignored by Selection. */
    return FVM_OK;                                                                            /* Preserve a successful selection. */
} /* The callback's borrowed values expire on return unless explicitly rooted. */


/* Observe both endpoints; SELECTION.md#selection-listeners defines the snapshot delivery policy. */
static FVM_Result focus_observer(FVM_Context *context, FVM_Object *receiver, unsigned int count, /* Borrow state. */
                                 const FVM_Value *args, FVM_Value *out, void *userdata)          /* Receive old/new clips. */
{                                                                                                /* This callback is independent of the MovieClip-specific handlers above. */
    (void)context;                                                                               /* Keep the example free of nested VM calls. */
    (void)receiver;                                                                              /* The receiver is the registered observer object. */
    (void)userdata;                                                                              /* No external state is needed. */
    if (count != 2)                                                                              /* Treat a broken host invocation as a real API error. */
    {                                                                                            /* Begin conditional branch: Treat a broken host invocation as a real API error. */
        return FVM_INVALID_ARGUMENT;                                                             /* Selection propagates errors after publishing focus. */
    } /* End conditional branch: Treat a broken host invocation as a real API error. */
    printf("observer old-null=%d new-null=%d\n",                            /* Print the two endpoint identities as null flags. */
           args[0].type == FVM_VALUE_NULL, args[1].type == FVM_VALUE_NULL); /* Do not retain borrowed values. */
    fvm_value_undefined(out);                                               /* Event return values do not change the selection result. */
    return FVM_OK;                                                          /* Continue synchronous notification delivery. */
} /* End the observer callback. */


/* Run all managed operations on one owning thread without opening an OS window. */
int main(void)                                          /* Use a strict-C89 host entry point. */
{                                                       /* Begin the complete ownership example. */
    FVM_Config config;                                  /* Own configuration storage. */
    FVM_Runtime *runtime = 0;                           /* Acquire before creating the context. */
    FVM_Context *context = 0;                           /* Own all managed objects and keyboard selection. */
    FVM_Object *observer = 0;                           /* Borrow an observer retained by its pin and registration. */
    int observer_pinned = 0;                            /* Track construction ownership even on partial setup failure. */
    FVM_Value observer_value;                           /* Pass one observer to addListener and removeListener. */
    FVM_Object *clip = 0;                               /* Borrow the clip while pinned or selected. */
    FVM_Value selection, argument, result;              /* Borrow namespace, argument and result values. */
    int pinned = 0;                                     /* Track whether cleanup owns a temporary root. */
    FVM_Result status = fvm_config_init(&config);       /* Initialize configuration defaults. */
    if (status == FVM_OK)                               /* Acquire dependent owners only after success. */
    {                                                   /* Begin conditional branch: Acquire dependent owners only after success. */
        status = fvm_runtime_create(&config, &runtime); /* Create the runtime owner. */
    } /* End conditional branch: Acquire dependent owners only after success. */
    if (status == FVM_OK)                               /* Isolate this host's globals and focus. */
    {                                                   /* Begin conditional branch: Isolate this host's globals and focus. */
        status = fvm_context_create(runtime, &context); /* Create one VM context. */
    } /* End conditional branch: Isolate this host's globals and focus. */
    if (status == FVM_OK)                       /* Install MovieClip and ordinary Object methods first. */
    {                                           /* Begin conditional branch: Install MovieClip and ordinary Object methods first. */
        status = fvm_standard_install(context); /* Preserve conflicting globals as errors. */
    } /* End conditional branch: Install MovieClip and ordinary Object methods first. */
    if (status == FVM_OK)                                 /* Explicitly opt into the supported Selection subset. */
    {                                                     /* Begin conditional branch: Explicitly opt into the supported Selection subset. */
        status = fvm_standard_selection_install(context); /* Install the explicit Selection namespace. */
    } /* End conditional branch: Explicitly opt into the supported Selection subset. */
    if (status == FVM_OK)                                              /* Create a standalone scene with no button-like handlers. */
    {                                                                  /* Begin conditional branch: Create a standalone scene with no button-like handlers. */
        status = fvm_movieclip_create(context, "hostMovie", 1, &clip); /* Return a borrowed clip. */
    } /* End conditional branch: Create a standalone scene with no button-like handlers. */
    if (status == FVM_OK)                   /* Protect construction before focus can retain the clip. */
    {                                       /* Begin temporary root acquisition. */
        status = fvm_gc_pin(context, clip); /* [GC:] Keep the native scene alive during setup. */
        pinned = status == FVM_OK;          /* Release this root only when acquisition succeeded. */
    } /* End temporary root acquisition. */
    if (status == FVM_OK)                                          /* Fetch the explicitly installed script namespace. */
    {                                                              /* Begin conditional branch: Fetch the explicitly installed script namespace. */
        status = fvm_get_global(context, "Selection", &selection); /* Borrow its managed object. */
    } /* End conditional branch: Fetch the explicitly installed script namespace. */
    fvm_value_undefined(&argument);                                                             /* Initialize every field before making an object value. */
    argument.type = FVM_VALUE_OBJECT;                                                           /* Pass an object rather than an activation-relative path. */
    argument.object = clip;                                                                     /* The pin protects this borrowed argument. */
    if (status == FVM_OK)                                                                       /* Ordinary clips are initially ineligible for script selection. */
    {                                                                                           /* Begin conditional branch: Ordinary clips are initially ineligible for script selection. */
        status = fvm_object_call(context, selection.object, "setFocus", 1, &argument, &result); /* Try once. */
    } /* End conditional branch: Ordinary clips are initially ineligible for script selection. */
    if (status == FVM_OK)                                /* Show a normal Boolean failure rather than an API error. */
    {                                                    /* Begin conditional branch: Show a normal Boolean failure rather than an API error. */
        printf("default accepted=%d\n", result.boolean); /* Expect zero for this non-button clip. */
    } /* End conditional branch: Show a normal Boolean failure rather than an API error. */
    fvm_value_boolean(&result, 1);                                       /* Prepare the script focusEnabled flag. */
    if (status == FVM_OK)                                                /* Make this MovieClip eligible without adding a mouse handler. */
    {                                                                    /* Begin conditional branch: Make this MovieClip eligible without adding a mouse handler. */
        status = fvm_object_set(context, clip, "focusEnabled", &result); /* Publish the flag. */
    } /* End conditional branch: Make this MovieClip eligible without adding a mouse handler. */
    if (status == FVM_OK)                                                                  /* Receive synchronous acquisition through the ordinary rooted method ABI. */
    {                                                                                      /* Begin conditional branch: Receive synchronous acquisition through the ordinary rooted method ABI. */
        status = fvm_object_define_method_rooted(context, clip, "onSetFocus", focus_event, /* Install callback. */
                                                 (void *)"set", 0);                        /* Borrow static userdata; no capture root. */
    } /* End conditional branch: Receive synchronous acquisition through the ordinary rooted method ABI. */
    if (status == FVM_OK)                                                                   /* Receive synchronous loss with the replacement clip or null as argument. */
    {                                                                                       /* Begin conditional branch: Receive synchronous loss with the replacement clip or null as argument. */
        status = fvm_object_define_method_rooted(context, clip, "onKillFocus", focus_event, /* Install callback. */
                                                 (void *)"kill", 0);                        /* Ordinary native GC barrier remains active. */
    } /* End conditional branch: Receive synchronous loss with the replacement clip or null as argument. */
    if (status == FVM_OK)                               /* Prepare a separate object for global Selection notifications. */
    {                                                   /* Begin conditional branch: Prepare a separate object for global Selection notifications. */
        status = fvm_object_create(context, &observer); /* Return a borrowed managed object. */
    } /* End conditional branch: Prepare a separate object for global Selection notifications. */
    if (status == FVM_OK)                       /* Protect partial observer setup before registration. */
    {                                           /* Begin observer pin acquisition. */
        status = fvm_gc_pin(context, observer); /* [GC:] Retain the observer until cleanup. */
        observer_pinned = status == FVM_OK;     /* Record only a successfully acquired pin. */
    } /* End observer pin acquisition. */
    if (status == FVM_OK)                                                         /* Selection observers receive two endpoint arguments. */
    {                                                                             /* Begin conditional branch: Selection observers receive two endpoint arguments. */
        status = fvm_object_define_method_rooted(context, observer, "onSetFocus", /* Register native code. */
                                                 focus_observer, 0, 0);           /* No userdata/capture ownership. */
    } /* End conditional branch: Selection observers receive two endpoint arguments. */
    fvm_value_undefined(&observer_value);                                     /* Initialize the observer argument completely. */
    observer_value.type = FVM_VALUE_OBJECT;                                   /* Select the ordinary managed-object representation. */
    observer_value.object = observer;                                         /* Construction pin retains this borrowed pointer. */
    if (status == FVM_OK)                                                     /* Add one strong registration, independent of the public namespace binding. */
    {                                                                         /* Begin conditional branch: Add one strong registration, independent of the public namespace binding. */
        status = fvm_object_call(context, selection.object, "addListener", 1, /* Register once by identity. */
                                 &observer_value, &result);                   /* Return value is undefined on success. */
    } /* End conditional branch: Add one strong registration, independent of the public namespace binding. */
    if (status == FVM_OK)                                                                       /* Try the same object with the updated eligibility. */
    {                                                                                           /* Begin conditional branch: Try the same object with the updated eligibility. */
        status = fvm_object_call(context, selection.object, "setFocus", 1, &argument, &result); /* Select it. */
    } /* End conditional branch: Try the same object with the updated eligibility. */
    if (status == FVM_OK)                                /* A successful selection is now a strong context GC root too. */
    {                                                    /* Begin conditional branch: A successful selection is now a strong context GC root too. */
        printf("enabled accepted=%d\n", result.boolean); /* Expect one. */
    } /* End conditional branch: A successful selection is now a strong context GC root too. */
    if (status == FVM_OK)                                                               /* Retrieve a counted native path rather than invoking clip.toString. */
    {                                                                                   /* Begin conditional branch: Retrieve a counted native path rather than invoking clip.toString. */
        status = fvm_object_call(context, selection.object, "getFocus", 0, 0, &result); /* Borrow the path. */
    } /* End conditional branch: Retrieve a counted native path rather than invoking clip.toString. */
    if (status == FVM_OK && result.type == FVM_VALUE_STRING)              /* Inspect only a real path value. */
    {                                                                     /* Print counted bytes while the borrowed result is live. */
        printf("focus=");                                                 /* Introduce the path. */
        fwrite(result.string, 1, fvm_value_string_size(&result), stdout); /* Preserve counted UTF-8. */
        printf("\n");                                                     /* Finish the line; a standalone root is _level0. */
    } /* End path output. */
    fvm_value_undefined(&argument);                                                             /* Undefined is the script-level request to remove focus. */
    if (status == FVM_OK)                                                                       /* Clear the host-local focus root before finishing. */
    {                                                                                           /* Begin conditional branch: Clear the host-local focus root before finishing. */
        status = fvm_object_call(context, selection.object, "setFocus", 1, &argument, &result); /* Clear it. */
    } /* End conditional branch: Clear the host-local focus root before finishing. */
    if (status == FVM_OK)                                                               /* Confirm that querying an empty selection returns null. */
    {                                                                                   /* Begin conditional branch: Confirm that querying an empty selection returns null. */
        status = fvm_object_call(context, selection.object, "getFocus", 0, 0, &result); /* Query again. */
    } /* End conditional branch: Confirm that querying an empty selection returns null. */
    if (status == FVM_OK)                                      /* Report the expected empty result without assertions. */
    {                                                          /* Begin conditional branch: Report the expected empty result without assertions. */
        printf("cleared=%d\n", result.type == FVM_VALUE_NULL); /* Expect one. */
    } /* End conditional branch: Report the expected empty result without assertions. */
    if (status == FVM_OK)                                                        /* Remove the registration explicitly before destroying its owner. */
    {                                                                            /* Begin conditional branch: Remove the registration explicitly before destroying its owner. */
        status = fvm_object_call(context, selection.object, "removeListener", 1, /* Remove by identity. */
                                 &observer_value, &result);                      /* Ordinary errors remain visible below. */
    } /* End conditional branch: Remove the registration explicitly before destroying its owner. */
    if (status == FVM_OK)                                /* Show successful removal rather than silently retaining an observer. */
    {                                                    /* Begin conditional branch: Show successful removal rather than silently retaining an observer. */
        printf("observer removed=%d\n", result.boolean); /* Expect one. */
    } /* End conditional branch: Show successful removal rather than silently retaining an observer. */
    if (status != FVM_OK)                                               /* Keep runtime errors visible to the embedding author. */
    {                                                                   /* Begin conditional branch: Keep runtime errors visible to the embedding author. */
        fprintf(stderr, "Selection example failed: %d\n", (int)status); /* Print the actual status. */
    } /* End conditional branch: Keep runtime errors visible to the embedding author. */
    if (observer_pinned)                 /* Release construction ownership on every exit path. */
    {                                    /* Begin conditional branch: Release construction ownership on every exit path. */
        fvm_gc_unpin(context, observer); /* [GC:] Context teardown releases any surviving registration. */
    } /* End conditional branch: Release construction ownership on every exit path. */
    if (pinned)                      /* Release the construction pin on success and failure. */
    {                                /* Begin conditional branch: Release the construction pin on success and failure. */
        fvm_gc_unpin(context, clip); /* [GC:] Context teardown still owns all remaining objects. */
    } /* End conditional branch: Release the construction pin on success and failure. */
    if (context)                      /* Destroy focus and any partial setup after the last borrowed access. */
    {                                 /* Begin conditional branch: Destroy focus and any partial setup after the last borrowed access. */
        fvm_context_destroy(context); /* End the VM owner lifetime. */
    } /* End conditional branch: Destroy focus and any partial setup after the last borrowed access. */
    if (runtime)                      /* Release the runtime after its context. */
    {                                 /* Begin conditional branch: Release the runtime after its context. */
        fvm_runtime_destroy(runtime); /* Complete cleanup. */
    } /* End conditional branch: Release the runtime after its context. */
    return status == FVM_OK ? 0 : 1; /* Return a normal shell success/error result. */
} /* End the standalone tutorial. */
