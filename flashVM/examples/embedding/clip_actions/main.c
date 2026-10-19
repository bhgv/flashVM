/* C89 tutorial: compare selected ClipActions, retained tree input and optional clip methods; see README.md and CLIP_ACTIONS.md. */
#include "flashvm/movieclip.h" /* Load the SWF graph and perform a normal timeline tick. */
#include "flashvm/clip_actions.h" /* Select one retained event without hit testing or native onX calls. */
#include "flashvm/standard.h" /* Install standard classes before scene construction. */
#include "flashvm/keyboard.h" /* Copy keyboard state and translate a held key to a SWF selector. */
#include "flashvm/input.h" /* Publish world pointer coordinates before retained input delivery. */
#include "flashvm/stage.h" /* Consume a redraw request without claiming any displayed pixels. */
#include "flashvm/gc.h" /* Pin the root while retaining borrowed scene pointers. */
#include "clip_action_movie.h" /* Embed an independently encoded, immutable SWF8 fixture. */
#include <stdio.h> /* Report the selected handler's script-visible effect. */
/* Observe a retained record before the optional native MovieClip method. */
static FVM_Result input_method(FVM_Context *context, FVM_Object *self, unsigned int argc, const FVM_Value *argv, FVM_Value *out, void *userdata) /* A synchronous owner-thread native callback. */
{ /* The selected scene retains self throughout this call. */
    FVM_Value seen; /* Borrow the script marker only until this callback returns. */
    FVM_Result status = fvm_object_get(context, self, "seen", &seen); /* Retained MouseMove has already written 84. */
    (void)argc; /* Named input methods receive no arguments. */
    (void)argv; /* Pointer coordinates are read through the native input APIs when needed. */
    (void)userdata; /* This callback owns no external capture. */
    if (status == FVM_OK && (seen.type != FVM_VALUE_NUMBER || seen.number != FVM_TUTORIAL_CLIP_INPUT_EXPECTED)) /* Validate record-before-method ordering. */
        status = FVM_INVALID_ARGUMENT; /* Preserve a mismatch through the outer delivery. */
    if (status == FVM_OK) /* Publish the method's own independently checked marker. */
    { /* Keep both native and script error paths explicit. */
        fvm_value_number(&seen, FVM_TUTORIAL_CLIP_METHOD_EXPECTED); /* The method changes the marker to 126. */
        status = fvm_object_set(context, self, "seen", &seen); /* This effect is committed even if a later operation fails. */
    } /* Finish the method's marker update. */
    if (status == FVM_OK) /* Request one eligible redraw for the whole batch. */
        status = fvm_call(context, "updateAfterEvent", 0, 0, out); /* The embedding host still owns presentation. */
    return status; /* Propagate method errors without retrying the event. */
} /* End the optional native MovieClip method. */
/* The host selects the actor; explicit delivery neither advances timeline nor opens a device. */
int main(void) /* Build this file as its own C89 executable, separate from main.c. */
{ /* Initialize partial owners before acquisition. */
    FVM_Config config; /* Runtime creation copies the initialized configuration. */
    FVM_Runtime *runtime = 0; /* Outlive its managed context. */
    FVM_Context *context = 0; /* Own the retained SWF library and handlers. */
    FVM_Object *root = 0, *actor = 0; /* Borrow graph objects, retaining them through the pinned root. */
    FVM_KeyboardState keyboard; /* The native copied snapshot owns no managed references. */
    unsigned int key_code = 0; /* Receive a SWF selector, distinct from Key.LEFT virtual code 37. */
    int redraw = 0; /* Receive and consume the MouseMove record's coalesced request. */
    FVM_Value value; /* Borrow each property result until its next use or context destruction. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the configuration ABI. */
    if (status == FVM_OK) /* Acquire the outer owner first. */
        status = fvm_runtime_create(&config, &runtime); /* No worker, input adapter or window is started. */
    if (status == FVM_OK) /* Establish the owner-thread managed heap. */
        status = fvm_context_create(runtime, &context); /* Use this thread for all following VM calls. */
    if (status == FVM_OK) /* Install standard classes before constructing scene instances. */
        status = fvm_standard_install(context); /* Preserve installation errors for cleanup. */
    if (status == FVM_OK) /* Decode, validate and copy placement-specific event records without executing them. */
        status = fvm_movieclip_load_swf(context, fvm_tutorial_clip_action_movie, sizeof(fvm_tutorial_clip_action_movie), "root", &root); /* Input bytes need not remain alive after return. */
    if (status == FVM_OK) /* Retain the complete library/scene graph across host boundaries. */
        status = fvm_gc_pin(context, root); /* [GC:] Context destruction releases this tutorial pin. */
    if (status == FVM_OK) /* Place the sprite and deliver its automatic EnterFrame record. */
        status = fvm_movieclip_advance(context, root, 1); /* The marker becomes 17; Load is not automatic. */
    if (status == FVM_OK) /* Resolve the host-selected instance by its authored name. */
        status = fvm_object_get(context, root, "actor", &value); /* No hit testing or input-state update occurs. */
    if (status == FVM_OK && (value.type != FVM_VALUE_OBJECT || !value.object)) /* Reject a missing or unexpected binding. */
        status = FVM_INVALID_ARGUMENT; /* Avoid dereferencing a nonobject property. */
    if (status == FVM_OK) /* Keep the borrowed actor reachable through root. */
    { /* Explicitly deliver the Load record once for this host integration. */
        actor = value.object; /* Do not unpin root while retaining this pointer. */
        status = fvm_movieclip_dispatch_clip_actions(context, actor, FVM_CLIP_EVENT_LOAD, 0); /* Non-KeyPress events require key_code zero. */
    } /* A second delivery would execute Load again; there is no hidden once-only cursor. */
    if (status == FVM_OK) /* Read the completed action's own marker. */
        status = fvm_object_get(context, actor, "seen", &value); /* The result is a plain scalar in this fixture. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != FVM_TUTORIAL_CLIP_ACTION_EXPECTED)) /* Verify the chosen record ran. */
        status = FVM_INVALID_ARGUMENT; /* Keep a mismatch observable through the exit code. */
    if (status == FVM_OK) /* Report the deterministic selected-event effect. */
        printf("explicit Load marker: %.0f\n", value.number); /* Expected output is 42, with no device required. */
    if (status == FVM_OK) /* Pointer publication and namespace listeners remain a separate host policy. */
        status = fvm_input_set_pointer(context, 20, 30); /* Set native world pixels without delivering callbacks. */
    if (status == FVM_OK) /* Capture the selected scene, then deliver global retained MouseMove records. */
        status = fvm_movieclip_dispatch_clip_input(context, root, FVM_CLIP_EVENT_MOUSE_MOVE); /* No hit test, native onX or frame step occurs. */
    if (status == FVM_OK) /* Verify the actor reached by the tree broadcast. */
        status = fvm_object_get(context, actor, "seen", &value); /* The borrowed actor remains reachable through root. */
    if (status == FVM_OK) /* Consume before a real embedding host would render. */
        status = fvm_stage_take_update(context, &redraw); /* This tutorial has no presenter and displays no pixels. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != FVM_TUTORIAL_CLIP_INPUT_EXPECTED || redraw != 1)) /* Check marker and eligible request together. */
        status = FVM_INVALID_ARGUMENT; /* Preserve a mismatch through ordinary cleanup. */
    if (status == FVM_OK) /* Report the second event independently of Load. */
        printf("scene MouseMove marker: %.0f; redraw: %d\n", value.number, redraw); /* Expected marker 84 and one coalesced request. */
    if (status == FVM_OK) /* Install a method without changing the immutable SWF records. */
        status = fvm_object_define_method_rooted(context, actor, "onMouseMove", input_method, 0, 0); /* No native capture needs a separate managed root. */
    if (status == FVM_OK) /* Deliver records and the optional method together through the same tree. */
        status = fvm_movieclip_dispatch_input(context, root, FVM_CLIP_EVENT_MOUSE_MOVE); /* Retained-only API above remains available. */
    if (status == FVM_OK) /* Read the method marker after the retained record wrote 84 again. */
        status = fvm_object_get(context, actor, "seen", &value); /* Expected method marker 126. */
    if (status == FVM_OK) /* Consume the coalesced record/method redraw request. */
        status = fvm_stage_take_update(context, &redraw); /* No timeline step or renderer is needed. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != FVM_TUTORIAL_CLIP_METHOD_EXPECTED || redraw != 1)) /* Verify marker and redraw together. */
        status = FVM_INVALID_ARGUMENT; /* Keep tutorial failures observable to its caller. */
    if (status == FVM_OK) /* Report the combined route independently of both older APIs. */
        printf("scene MouseMove method: %.0f; redraw: %d\n", value.number, redraw); /* Expected marker 126 and one request. */
    keyboard.struct_size = sizeof(keyboard); /* Describe the copied-keyboard ABI before its getter. */
    if (status == FVM_OK) /* Borrow no native input buffer: the getter copies every field. */
        status = fvm_input_get_keyboard(context, &keyboard); /* Preserve any getter error for cleanup. */
    if (status == FVM_OK) /* Prepare a synthetic held Left transition without opening an OS device. */
    { /* The caller supplies transitions when embedding without SDL. */
        keyboard.down[FVM_KEY_LEFT] = 1; /* Mark the virtual key held before publishing. */
        keyboard.last_code = FVM_KEY_LEFT; /* Virtual code 37 translates to SWF KeyPress code 1. */
        keyboard.last_ascii = 0; /* Special keys do not need a printable character. */
        status = fvm_input_set_keyboard(context, &keyboard); /* Publish before retained input callbacks. */
    } /* End host snapshot construction. */
    if (status == FVM_OK) /* Translate this copied key-down, without changing context state. */
        status = fvm_input_clip_key_code(&keyboard, 1, &key_code); /* A zero result means skip, not a wildcard. */
    if (status == FVM_OK && key_code == FVM_CLIP_KEY_NONE) /* This known Left key must translate. */
        status = FVM_INVALID_ARGUMENT; /* Avoid accidentally selecting a raw zero record. */
    if (status == FVM_OK) /* Broadcast only the matching retained KeyPress records. */
        status = fvm_movieclip_dispatch_key_press(context, root, key_code); /* No onKeyPress method or global KeyDown is implied. */
    if (status == FVM_OK) /* Verify the fixture's raw selector, not virtual code 37. */
        status = fvm_object_get(context, actor, "seen", &value); /* Expected KeyPress marker 168. */
    if (status == FVM_OK) /* Consume the request produced by the host-input KeyPress scope. */
        status = fvm_stage_take_update(context, &redraw); /* Presentation remains the embedding host's task. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != FVM_TUTORIAL_CLIP_KEY_EXPECTED || redraw != 1)) /* Verify marker and request together. */
        status = FVM_INVALID_ARGUMENT; /* Preserve a mismatch without replaying the transition. */
    if (status == FVM_OK) /* Report this fourth delivery independently of the previous routes. */
        printf("scene KeyPress key: %u; marker: %.0f; redraw: %d\n", key_code, value.number, redraw); /* Expected 1, 168 and one request. */
    else /* Preserve failures without assertions or automatic replay. */
        fprintf(stderr, "ClipActions tutorial failed: %d\n", (int)status); /* A failed batch may already have script effects. */
    if (context) /* Release the pinned root, library and code before host locals expire. */
        fvm_context_destroy(context); /* [GC:] No managed pointer is used afterwards. */
    if (runtime) /* Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* No platform device or worker is owned. */
    return status == FVM_OK ? 0 : 1; /* Permit a shell or CTest to check the result. */
} /* End the explicit owner-thread host. */
