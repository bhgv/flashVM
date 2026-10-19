/* C89 tutorial: deliver retained sprite ClipActions explicitly; see README.md and CLIP_ACTIONS.md. */
#include "flashvm/movieclip.h" /* Load the SWF graph and perform a normal timeline tick. */
#include "flashvm/clip_actions.h" /* Select one retained event without hit testing or native onX calls. */
#include "flashvm/standard.h" /* Install standard classes before scene construction. */
#include "flashvm/input.h" /* Publish world pointer coordinates before retained input delivery. */
#include "flashvm/stage.h" /* Consume a redraw request without claiming any displayed pixels. */
#include "flashvm/gc.h" /* Pin the root while retaining borrowed scene pointers. */
#include "clip_action_movie.h" /* Embed an independently encoded, immutable SWF8 fixture. */
#include <stdio.h> /* Report the selected handler's script-visible effect. */
/* The host selects the actor; explicit delivery neither advances timeline nor opens a device. */
int main(void) /* Build this file as its own C89 executable, separate from main.c. */
{ /* Initialize partial owners before acquisition. */
    FVM_Config config; /* Runtime creation copies the initialized configuration. */
    FVM_Runtime *runtime = 0; /* Outlive its managed context. */
    FVM_Context *context = 0; /* Own the retained SWF library and handlers. */
    FVM_Object *root = 0, *actor = 0; /* Borrow graph objects, retaining them through the pinned root. */
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
    else /* Preserve failures without assertions or automatic replay. */
        fprintf(stderr, "ClipActions tutorial failed: %d\n", (int)status); /* A failed batch may already have script effects. */
    if (context) /* Release the pinned root, library and code before host locals expire. */
        fvm_context_destroy(context); /* [GC:] No managed pointer is used afterwards. */
    if (runtime) /* Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* No platform device or worker is owned. */
    return status == FVM_OK ? 0 : 1; /* Permit a shell or CTest to check the result. */
} /* End the explicit owner-thread host. */
