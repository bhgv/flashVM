/* Real Clutter/Cogl texture and event tutorial, compatible with the shell's public actor APIs. */
#include "../common/demo.h" /* Share only the backend-independent VM/frame lifecycle. */
#include <clutter/clutter.h> /* Explicit Clutter SDK, selected by the embedding application. */
#include <stdio.h> /* Report real SDK/VM errors and bounded smoke results. */
/* Stage owns the texture actor; the timer borrows this host only until it is removed. */
typedef struct ClutterHost /* No client code from inferno-os is used. */
{ /* Keep callback lifetimes explicit. */
    Demo demo; /* Owned VM and RGBA bytes, destroyed after callbacks stop. */
    ClutterActor *stage; /* Explicitly retained stage owner through cleanup. */
    ClutterActor *texture; /* Borrowed child actor owned by the stage. */
    ClutterEventSequence *sequence; /* Borrowed identity of the active first touch, never dereferenced. */
    guint timer; /* Owned GLib source ID, zero after its callback removes itself. */
    int destroyed; /* Stage destruction invalidates the borrowed child actor. */
    int looping; /* Nonzero only while this launcher owns an active Clutter main loop. */
    int smoke; /* Explicit bounded run, otherwise wait for native stage close. */
    int touch_only; /* Suppress every mouse event for shells that synthesize mouse from touch. */
} ClutterHost; /* Main owns this structure until all signal/timer callbacks stop. */
/* Upload RGBA with a byte stride into the actor's Cogl-backed texture. */
static FVM_Result clutter_present(const FVM_RasterSurface *frame, void *userdata) /* Borrow frame. */
{ /* Run only on the Clutter/VM owner thread with a live graphics context. */
    ClutterHost *host = (ClutterHost *)userdata; /* Borrow native owners. */
    GError *error = NULL; /* Own any SDK diagnostic until g_error_free. */
    gboolean uploaded; /* Keep the actual library result. */
    if (host->destroyed) /* Never upload after stage destroyed its children. */
        return FVM_ERROR; /* Owner loop performs cleanup. */
    uploaded = clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(host->texture), /* Actual texture actor. */
                 frame->pixels, TRUE, frame->width, frame->height, /* RGBA has alpha, top-left rows. */
                 (gint)frame->stride, FVM_HOST_RGBA_BYTES, CLUTTER_TEXTURE_NONE, &error); /* Straight RGBA. */
    if (error) /* Report and release the independently allocated SDK error. */
    { /* Do not retain a diagnostic across frames. */
        fprintf(stderr, "Clutter upload: %s\n", error->message); /* Borrow text until free below. */
        g_error_free(error); /* Release SDK-owned error storage exactly once. */
    } /* Successful uploads normally leave error NULL. */
    return uploaded ? FVM_OK : FVM_ERROR; /* Cogl/Clutter retains its own image copy. */
} /* The caller may reuse borrowed source pixels after return. */
/* Follow stage allocation without changing VM world coordinates. */
static void clutter_size(ClutterActor *stage, const ClutterActorBox *box, /* Signal arguments. */
                          ClutterAllocationFlags flags, gpointer userdata) /* Borrow callback state. */
{ /* Clutter performs image scaling; input applies its inverse once. */
    ClutterHost *host = (ClutterHost *)userdata; /* Main owns state. */
    (void)stage; (void)flags; /* Allocation box supplies the relevant new size. */
    clutter_actor_set_size(host->texture, box->x2 - box->x1, box->y2 - box->y1); /* Fill this stage. */
} /* Embedded shells may instead allocate/transform the texture actor themselves. */
/* Finish stage-owned callbacks before main releases VM state. */
static void clutter_destroy(ClutterActor *stage, gpointer userdata) /* Borrow signal data. */
{ /* The explicit reference keeps the destroyed stage object valid for final unref. */
    ClutterHost *host = (ClutterHost *)userdata; /* State remains alive until main returns. */
    (void)stage; /* Host already records the retained stage. */
    host->destroyed = 1; /* Its borrowed child must not be used again. */
    host->texture = NULL; /* Prevent accidental upload after destruction. */
    if (host->looping) clutter_main_quit(); /* Return control to ordered main cleanup. */
} /* Main removes any remaining timer before releasing demo owners. */
/* Native stage deactivation is a stage signal, independent of captured pointer events. */
static void clutter_deactivate(ClutterStage *stage, gpointer userdata) /* Borrow signal state. */
{ /* Avoid relying on stage-state events passing through the capture phase. */
    ClutterHost *host = (ClutterHost *)userdata; /* Keep cancellation on the VM owner thread. */
    (void)stage; /* Last valid VM coordinates are sufficient for cancellation. */
    demo_cancel(&host->demo); /* Explicit up policy, no untrusted native coordinates. */
    host->sequence = NULL; /* Release native first-contact ownership as well. */
} /* Signal removal follows the stage lifetime. */
/* Translate one stage-space pointer/touch event into the texture's local/world space. */
static gboolean clutter_input(ClutterActor *stage, ClutterEvent *event, gpointer userdata) /* Captured signal. */
{ /* A transformed actor requires the SDK's inverse projected stage mapping. */
    ClutterHost *host = (ClutterHost *)userdata; /* Borrow main-thread state. */
    ClutterEventType native_type = clutter_event_type(event); /* Preserve the native event kind. */
    ClutterEventSequence *sequence; /* Compare touch identities without dereferencing them. */
    FVM_MouseEventType type; /* Only recognized events reach the VM. */
    FVM_Affine world_to_local = {1, 0, 0, 1, 0, 0}; /* Actor-local scale from fixed world pixels. */
    gfloat stage_x, stage_y, local_x, local_y; /* Clutter's event and projected actor coordinates. */
    (void)stage; /* The texture actor is the actual event coordinate target. */
    if (native_type == CLUTTER_TOUCH_CANCEL) /* Cancel coordinates may be absent or invalid. */
    { /* Only the selected contact can cancel the primary pointer. */
        if (host->sequence == clutter_event_get_event_sequence(event)) /* Match the active sequence. */
        { /* Release both VM/native contact state. */
            demo_cancel(&host->demo); /* Never map untrusted cancel coordinates. */
            host->sequence = NULL; /* No secondary promotion. */
        } /* Other contact cancellations leave the primary alone. */
        return CLUTTER_EVENT_STOP; /* This dedicated stage consumes tutorial touch input. */
    } /* End cancellation handling. */
    if (native_type == CLUTTER_TOUCH_BEGIN || native_type == CLUTTER_TOUCH_UPDATE || /* Touch stream. */
        native_type == CLUTTER_TOUCH_END) /* Match identities through the final event. */
    { /* Retain no event object, only its active sequence identity. */
        sequence = clutter_event_get_event_sequence(event); /* Borrow valid sequence identity. */
        if (!sequence) /* Invalid touch input must not become an anonymous primary contact. */
            return CLUTTER_EVENT_STOP; /* Wait for a valid native event. */
        if (native_type == CLUTTER_TOUCH_BEGIN && !host->sequence && !host->demo.held) /* First contact wins. */
            host->sequence = sequence; /* Held physical mouse prevents touch takeover. */
        if (host->sequence != sequence) /* Secondary contacts never move or end the primary. */
            return CLUTTER_EVENT_STOP; /* They are ignored until a new begin after primary end. */
        type = native_type == CLUTTER_TOUCH_BEGIN ? FVM_MOUSE_EVENT_DOWN : /* Preserve begin. */
               native_type == CLUTTER_TOUCH_END ? FVM_MOUSE_EVENT_UP : FVM_MOUSE_EVENT_MOVE; /* Update/end. */
    } /* End touch selection. */
    else if (!host->touch_only && !host->sequence && /* Never duplicate touch with pointer synthesis. */
             (native_type == CLUTTER_MOTION || native_type == CLUTTER_BUTTON_PRESS || /* Physical pointer. */
              native_type == CLUTTER_BUTTON_RELEASE)) /* Ignore wheel and unsupported input. */
    { /* Button one is the primary contact; right/middle do not change VM primary state. */
        if (native_type != CLUTTER_MOTION && clutter_event_get_button(event) != DEMO_PRIMARY_BUTTON) /* Primary only. */
            return CLUTTER_EVENT_PROPAGATE; /* Permit the containing shell to handle other buttons. */
        type = native_type == CLUTTER_BUTTON_PRESS ? FVM_MOUSE_EVENT_DOWN : /* Physical down. */
               native_type == CLUTTER_BUTTON_RELEASE ? FVM_MOUSE_EVENT_UP : FVM_MOUSE_EVENT_MOVE; /* Up/move. */
    } /* End physical-pointer selection. */
    else /* Keyboard, IME, gestures and shell policy are outside this tutorial. */
        return CLUTTER_EVENT_PROPAGATE; /* Do not consume unrelated shell input. */
    clutter_event_get_coords(event, &stage_x, &stage_y); /* Read actual stage-space pixels. */
    if (!clutter_actor_transform_stage_point(host->texture, stage_x, stage_y, &local_x, &local_y)) /* Invert projection. */
    { /* A singular/edge-on actor cannot produce a meaningful world coordinate. */
        demo_cancel(&host->demo); /* End held contact safely at its previous valid position. */
        host->sequence = NULL; /* Release native identity rather than leaving a stuck hold. */
        return CLUTTER_EVENT_STOP; /* Skip this unmappable native event explicitly. */
    } /* Actor local coordinates still include texture allocation scale. */
    world_to_local.xx = clutter_actor_get_width(host->texture) / DEMO_WIDTH; /* World-to-local X scale. */
    world_to_local.yy = clutter_actor_get_height(host->texture) / DEMO_HEIGHT; /* World-to-local Y scale. */
    demo_pointer(&host->demo, type, &world_to_local, local_x, local_y); /* Convert once, then dispatch. */
    if (native_type == CLUTTER_TOUCH_END) /* End only the contact selected above. */
        host->sequence = NULL; /* Additional fingers are never promoted. */
    return CLUTTER_EVENT_STOP; /* A dedicated demo stage has consumed this pointer transition. */
} /* No event or actor-local coordinate escapes the callback. */
/* Generate a frame on Clutter's main thread; stop bounded runs/errors cleanly. */
static gboolean clutter_tick(gpointer userdata) /* GLib borrows main-owned host until source removal. */
{ /* Never call the VM from a platform input worker. */
    ClutterHost *host = (ClutterHost *)userdata; /* Borrow one independent host. */
    demo_frame(&host->demo, clutter_present, host); /* Copy/upload before returning borrowed frame. */
    if (host->demo.status != FVM_OK || (host->smoke && host->demo.frames >= DEMO_MAX_STEPS)) /* Finish bounded run. */
    { /* This callback removes its own source by returning FALSE. */
        host->timer = 0; /* Main must not remove a stale source ID. */
        clutter_main_quit(); /* Resume main for ordered cleanup. */
        return FALSE; /* Stop further timer callbacks. */
    } /* Interactive mode continues until stage destruction. */
    return TRUE; /* Keep the timer attached to the same owning main context. */
} /* No asynchronous frame consumer remains after this callback. */
/* Desktop SDK launcher; an Android shell instead reuses the callbacks with its own stage/lifecycle. */
int main(int argc, char **argv) /* This is not an Android Activity or replacement native_app_glue. */
{ /* Initialize Clutter before creating any actor/context-dependent resources. */
    ClutterHost host; /* Main owns every callback's userdata. */
    int i, result = 1; /* Fail unless initialization and all rendered frames succeed. */
    memset(&host, 0, sizeof(host)); /* Partial cleanup has no uninitialized owners. */
    for (i = 1; i < argc; ++i) /* Parse only the tutorial's own options. */
    { /* Strip our options before passing argc/argv to Clutter. */
        if (!strcmp(argv[i], "--smoke")) host.smoke = 1; /* Stop after 120 submitted frames. */
        else if (!strcmp(argv[i], "--touch-only")) host.touch_only = 1; /* Ignore synthetic/physical mice entirely. */
        else /* Reject unknown arguments with a concrete usage message. */
        { /* No native resources exist yet. */
            fprintf(stderr, "usage: %s [--smoke] [--touch-only]\n", argv[0]); /* Interactive or bounded mode. */
            return 1; /* Do not silently ignore a misspelled option. */
        } /* End argument failure. */
    } /* Preserve argv[0], then let the SDK initialize without tutorial options. */
    argc = 1; /* Clutter receives no private command-line switches. */
    if (clutter_init(&argc, &argv) != CLUTTER_INIT_SUCCESS) /* Open actual native backend/graphics context. */
        return 1; /* SDK reports its initialization error, no owners exist. */
    if (demo_open(&host.demo) != FVM_OK) /* Create the platform-independent VM/frame owners. */
        goto cleanup; /* Preserve the VM failure. */
    host.stage = clutter_stage_new(); /* Create a distinct desktop stage for this tutorial. */
    g_object_ref_sink(host.stage); /* Own an explicit reference even after native destroy. */
    host.texture = clutter_texture_new(); /* Create the SDK-compatible Cogl-backed texture actor. */
    clutter_actor_add_child(host.stage, host.texture); /* Stage adopts the floating texture reference. */
    clutter_actor_set_size(host.stage, DEMO_WIDTH, DEMO_HEIGHT); /* Set initial native allocation. */
    clutter_actor_set_size(host.texture, DEMO_WIDTH, DEMO_HEIGHT); /* Initial image allocation. */
    clutter_stage_set_title(CLUTTER_STAGE(host.stage), "FlashVM Clutter tutorial"); /* Native title. */
    clutter_actor_set_reactive(host.stage, TRUE); /* Receive captured pointer/touch events. */
    g_signal_connect(host.stage, "captured-event", G_CALLBACK(clutter_input), &host); /* Borrow host. */
    g_signal_connect(host.stage, "allocation-changed", G_CALLBACK(clutter_size), &host); /* Track resize. */
    g_signal_connect(host.stage, "deactivate", G_CALLBACK(clutter_deactivate), &host); /* Cancel held contact. */
    g_signal_connect(host.stage, "destroy", G_CALLBACK(clutter_destroy), &host); /* Stop before child invalidation. */
    clutter_actor_show(host.stage); /* Make the real SDK window/actors visible. */
    host.timer = g_timeout_add(DEMO_FRAME_MS, clutter_tick, &host); /* Coarse frame pacing, not a movie/audio clock. */
    host.looping = 1; /* Destroy callback may now request this loop to quit. */
    clutter_main(); /* Deliver native events and timers on the single VM/graphics owner. */
    host.looping = 0; /* Cleanup destruction must not quit a nonexistent main loop. */
    result = host.demo.status == FVM_OK ? 0 : 1; /* Preserve backend and VM failures. */
    if (!result) printf("Clutter frames=%u\n", host.demo.frames); /* Smoke expects 120 submissions. */
cleanup: /* Stop callback sources before releasing their userdata or pixels. */
    if (host.timer) g_source_remove(host.timer); /* Remove outstanding timer first. */
    if (host.stage && !host.destroyed) clutter_actor_destroy(host.stage); /* Stop stage signals/children. */
    if (host.stage) g_object_unref(host.stage); /* Release the retained stage reference. */
    if (result) fprintf(stderr, "Clutter tutorial: VM=%d\n", (int)host.demo.status); /* Report first failure. */
    demo_close(&host.demo); /* No callback can access this VM/frame storage afterward. */
    return result; /* A desktop software run does not certify Android or physical GLES. */
} /* No dependency on the ignored inferno-os client folder. */
