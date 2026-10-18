/* Compare default rejection, explicit MovieClip eligibility and Selection focus clearing. */
#include "flashvm/keyboard.h" /* Declare the optional Selection installer. */
#include "flashvm/movieclip.h" /* Create a native MovieClip receiver. */
#include "flashvm/standard.h" /* Install standard constructors and prototypes. */
#include "flashvm/gc.h" /* Protect the clip before eligibility and focus are configured. */
#include <stdio.h> /* Print ordinary results and errors without tutorial assertions. */
/* Run all managed operations on one owning thread without opening an OS window. */
int main(void) /* Use a strict-C89 host entry point. */
{ /* Begin the complete ownership example. */
    FVM_Config config; /* Own configuration storage. */
    FVM_Runtime *runtime = 0; /* Acquire before creating the context. */
    FVM_Context *context = 0; /* Own all managed objects and keyboard selection. */
    FVM_Object *clip = 0; /* Borrow the clip while pinned or selected. */
    FVM_Value selection, argument, result; /* Borrow namespace, argument and result values. */
    int pinned = 0; /* Track whether cleanup owns a temporary root. */
    FVM_Result status = fvm_config_init(&config); /* Initialize configuration defaults. */
    if (status == FVM_OK) /* Acquire dependent owners only after success. */
        status = fvm_runtime_create(&config, &runtime); /* Create the runtime owner. */
    if (status == FVM_OK) /* Isolate this host's globals and focus. */
        status = fvm_context_create(runtime, &context); /* Create one VM context. */
    if (status == FVM_OK) /* Install MovieClip and ordinary Object methods first. */
        status = fvm_standard_install(context); /* Preserve conflicting globals as errors. */
    if (status == FVM_OK) /* Explicitly opt into the supported Selection subset. */
        status = fvm_standard_selection_install(context); /* Install getFocus and setFocus only. */
    if (status == FVM_OK) /* Create a standalone scene with no button-like handlers. */
        status = fvm_movieclip_create(context, "hostMovie", 1, &clip); /* Return a borrowed clip. */
    if (status == FVM_OK) /* Protect construction before focus can retain the clip. */
    { /* Begin temporary root acquisition. */
        status = fvm_gc_pin(context, clip); /* [GC:] Keep the native scene alive during setup. */
        pinned = status == FVM_OK; /* Release this root only when acquisition succeeded. */
    } /* End temporary root acquisition. */
    if (status == FVM_OK) /* Fetch the explicitly installed script namespace. */
        status = fvm_get_global(context, "Selection", &selection); /* Borrow its managed object. */
    fvm_value_undefined(&argument); /* Initialize every field before making an object value. */
    argument.type = FVM_VALUE_OBJECT; /* Pass an object rather than an activation-relative path. */
    argument.object = clip; /* The pin protects this borrowed argument. */
    if (status == FVM_OK) /* Ordinary clips are initially ineligible for script selection. */
        status = fvm_object_call(context, selection.object, "setFocus", 1, &argument, &result); /* Try once. */
    if (status == FVM_OK) /* Show a normal Boolean failure rather than an API error. */
        printf("default accepted=%d\n", result.boolean); /* Expect zero for this non-button clip. */
    fvm_value_boolean(&result, 1); /* Prepare the script focusEnabled flag. */
    if (status == FVM_OK) /* Make this MovieClip eligible without adding a mouse handler. */
        status = fvm_object_set(context, clip, "focusEnabled", &result); /* Publish the flag. */
    if (status == FVM_OK) /* Try the same object with the updated eligibility. */
        status = fvm_object_call(context, selection.object, "setFocus", 1, &argument, &result); /* Select it. */
    if (status == FVM_OK) /* A successful selection is now a strong context GC root too. */
        printf("enabled accepted=%d\n", result.boolean); /* Expect one. */
    if (status == FVM_OK) /* Retrieve a counted native path rather than invoking clip.toString. */
        status = fvm_object_call(context, selection.object, "getFocus", 0, 0, &result); /* Borrow the path. */
    if (status == FVM_OK && result.type == FVM_VALUE_STRING) /* Inspect only a real path value. */
    { /* Print counted bytes while the borrowed result is live. */
        printf("focus="); /* Introduce the path. */
        fwrite(result.string, 1, fvm_value_string_size(&result), stdout); /* Preserve counted UTF-8. */
        printf("\n"); /* Finish the line; a standalone root is _level0. */
    } /* End path output. */
    fvm_value_undefined(&argument); /* Undefined is the script-level request to remove focus. */
    if (status == FVM_OK) /* Clear the host-local focus root before finishing. */
        status = fvm_object_call(context, selection.object, "setFocus", 1, &argument, &result); /* Clear it. */
    if (status == FVM_OK) /* Confirm that querying an empty selection returns null. */
        status = fvm_object_call(context, selection.object, "getFocus", 0, 0, &result); /* Query again. */
    if (status == FVM_OK) /* Report the expected empty result without assertions. */
        printf("cleared=%d\n", result.type == FVM_VALUE_NULL); /* Expect one. */
    if (status != FVM_OK) /* Keep runtime errors visible to the embedding author. */
        fprintf(stderr, "Selection example failed: %d\n", (int)status); /* Print the actual status. */
    if (pinned) /* Release the construction pin on success and failure. */
        fvm_gc_unpin(context, clip); /* [GC:] Context teardown still owns all remaining objects. */
    if (context) /* Destroy focus and any partial setup after the last borrowed access. */
        fvm_context_destroy(context); /* End the VM owner lifetime. */
    if (runtime) /* Release the runtime after its context. */
        fvm_runtime_destroy(runtime); /* Complete cleanup. */
    return status == FVM_OK ? 0 : 1; /* Return a normal shell success/error result. */
} /* End the standalone tutorial. */
