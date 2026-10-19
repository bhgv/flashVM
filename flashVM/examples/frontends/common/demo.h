/* Shared C89 VM/frame lifecycle for the SDL and Clutter tutorials; see ../README.md. */
#ifndef FVM_FRONTEND_DEMO_H /* Keep each executable's private implementation independent. */
#define FVM_FRONTEND_DEMO_H /* No window SDK types enter the common host. */
#include "flashvm/host.h" /* Validate borrowed RGBA frames and inverse event coordinates. */
#include "flashvm/input.h" /* Query context-owned world pointer coordinates. */
#include "flashvm/mouse.h" /* Deliver primary-contact transitions on the VM owner thread. */
#include <stdlib.h> /* Own the pixel allocation until all callbacks have stopped. */
#include <string.h> /* Initialize state and clear the host-owned frame. */
#define DEMO_WIDTH 320U /* Fixed logical frame width; window/actor scaling is external. */
#define DEMO_HEIGHT 200U /* Fixed logical frame height in world pixels. */
#define DEMO_CORNERS 4U /* Number of vertices in the marker polygon. */
#define DEMO_MARKER_HALF 12.0 /* Half-size of the visible pointer marker in world pixels. */
#define DEMO_IDLE_RGB 40U /* Dark opaque background channel. */
#define DEMO_FRAME_MS 16U /* Coarse tutorial pacing in milliseconds. */
#define DEMO_PRIMARY_BUTTON 1U /* Primary pointer button in both external SDKs. */
#define DEMO_RGB_CHANNELS 3U /* RGB bytes before the straight-alpha byte. */
#define DEMO_OPAQUE 255U /* Fully opaque straight alpha or maximum color channel. */
#define DEMO_RELEASED_RED 40U /* Released marker red channel. */
#define DEMO_RELEASED_GREEN 220U /* Released marker green channel. */
#define DEMO_MARKER_BLUE 120U /* Marker blue channel in both contact states. */
#define DEMO_HELD_RED 240U /* Held marker red channel. */
#define DEMO_HELD_GREEN 80U /* Held marker green channel. */
#define DEMO_MAX_STEPS 120U /* Bounded software smoke-run length when --smoke is requested. */
/* [GC:] No managed object escapes; native state is owned by the calling application. */
typedef struct Demo /* One independent VM and frame per frontend instance. */
{ /* Start the plain host owner. */
    FVM_Runtime *runtime; /* Owned outer VM, destroyed after context. */
    FVM_Context *context; /* Owned context; all calls stay on its creating thread. */
    FVM_RasterSurface frame; /* Borrows pixels allocated by demo_open. */
    size_t byte_count; /* Actual accessible span used by fvm_host_present. */
    FVM_Result status; /* First failure stops rendering and event delivery. */
    int held; /* Primary-contact state used only for marker color/cancel policy. */
    unsigned int frames; /* Number of successfully submitted frames. */
} Demo; /* [GC:] No GC pin is needed because there are no exposed managed objects. */
/* Release VM owners and pixel storage after event/timer callbacks have stopped. */
static void demo_close(Demo *demo) /* Accept partially initialized ownership. */
{ /* Destroy dependencies in reverse acquisition order. */
    if (demo->context) /* Release only a successfully created context. */
        fvm_context_destroy(demo->context); /* No callbacks may use it after this call. */
    if (demo->runtime) /* The context is gone before the runtime. */
        fvm_runtime_destroy(demo->runtime); /* Release the outer VM owner. */
    free(demo->frame.pixels); /* No consumer retains borrowed bytes after return. */
    memset(demo, 0, sizeof(*demo)); /* Make repeated cleanup safe. */
} /* The caller still owns the Demo structure itself. */
/* Create portable state without SDL, Clutter, a device or script standard namespaces. */
static FVM_Result demo_open(Demo *demo) /* Caller provides fresh storage. */
{ /* Establish cleanup-safe state before any acquisition. */
    FVM_Config config; /* Initialize the public configuration before creating runtime. */
    memset(demo, 0, sizeof(*demo)); /* Clear all owners and counters. */
    demo->frame.width = DEMO_WIDTH; /* Keep world/image coordinates equal. */
    demo->frame.height = DEMO_HEIGHT; /* Presentation may scale the image independently. */
    demo->frame.stride = DEMO_WIDTH * FVM_HOST_RGBA_BYTES; /* Tightly packed RGBA rows. */
    demo->byte_count = demo->frame.stride * DEMO_HEIGHT; /* Bounded constants prevent overflow. */
    demo->frame.pixels = (unsigned char *)malloc(demo->byte_count); /* Retain until demo_close. */
    if (!demo->frame.pixels) /* Allocation failure must not become a blank successful window. */
        return demo->status = FVM_OUT_OF_MEMORY; /* Caller still invokes demo_close. */
    demo->status = fvm_config_init(&config); /* No VM is acquired if initialization fails. */
    if (demo->status == FVM_OK) /* Acquire the outer owner first. */
        demo->status = fvm_runtime_create(&config, &demo->runtime); /* Store ownership for cleanup. */
    if (demo->status == FVM_OK) /* Acquire an independent pointer context. */
        demo->status = fvm_context_create(demo->runtime, &demo->context); /* No window API enters VM. */
    if (demo->status == FVM_OK) /* Begin with a visible marker at frame center. */
        demo->status = fvm_input_set_pointer(demo->context, DEMO_WIDTH / 2, DEMO_HEIGHT / 2); /* World pixels. */
    return demo->status; /* Preserve the first real failure. */
} /* All acquired owners remain valid until demo_close. */
/* Publish one event after the backend has selected its first primary contact. */
static FVM_Result demo_pointer(Demo *demo, FVM_MouseEventType type, /* Down, move or up. */
                               const FVM_Affine *world_to_event, double x, double y) /* Borrow transform. */
{ /* Convert in the coordinate space actually delivered by the platform. */
    double world_x, world_y; /* Outputs are valid only after successful conversion. */
    if (demo->status != FVM_OK) /* Do not overwrite a previous backend/VM failure. */
        return demo->status; /* Owner loop will exit. */
    demo->status = fvm_host_pointer_to_world(world_to_event, x, y, &world_x, &world_y); /* No clipping. */
    if (demo->status == FVM_OK) /* Dispatch only successfully mapped finite coordinates. */
        demo->status = fvm_input_dispatch_mouse(demo->context, type, world_x, world_y); /* Synchronous VM entry. */
    if (demo->status == FVM_OK && type != FVM_MOUSE_EVENT_MOVE) /* Reflect accepted button transitions. */
        demo->held = type == FVM_MOUSE_EVENT_DOWN; /* A successful up clears marker color state. */
    return demo->status; /* Never silently drop mapping or VM errors. */
} /* There is no event worker or implicit queue. */
/* Explicit tutorial cancellation policy: finish a held contact at its last valid world position. */
static void demo_cancel(Demo *demo) /* Call on focus loss or native contact cancellation. */
{ /* This is a synthetic up policy, not an automatic Flash click/focus implementation. */
    double x, y; /* Preserve the last position rather than reading invalid cancel coordinates. */
    if (!demo->held || demo->status != FVM_OK) /* Nothing to cancel after an earlier error. */
        return; /* No VM entry is needed. */
    demo->status = fvm_input_get_pointer(demo->context, &x, &y); /* Read independent context state. */
    if (demo->status == FVM_OK) /* End only a successfully queried contact. */
        demo->status = fvm_input_dispatch_mouse(demo->context, FVM_MOUSE_EVENT_UP, x, y); /* Owner thread. */
    demo->held = 0; /* Native cancellation releases ownership even if dispatch fails. */
} /* Backend code also clears its native finger identity. */
/* Render from the VM pointer and submit through a host-provided synchronous consumer. */
static FVM_Result demo_frame(Demo *demo, FVM_HostPresentCallback present, void *userdata) /* Borrow sink. */
{ /* Rendering changes host pixels only, without modifying a script transform. */
    double x, y; /* World pointer determines the center of the visible square. */
    size_t offset; /* Traverse the tightly packed background pixel buffer. */
    FVM_RasterPoint points[DEMO_CORNERS]; /* Borrow rectangle vertices only during rasterization. */
    FVM_RasterColor color = {DEMO_RELEASED_RED, DEMO_RELEASED_GREEN, DEMO_MARKER_BLUE, DEMO_OPAQUE}; /* Green released marker; straight alpha. */
    if (demo->status != FVM_OK) /* Preserve the first failure across frames. */
        return demo->status; /* Stop the host event/timer loop. */
    demo->status = fvm_input_get_pointer(demo->context, &x, &y); /* Copy VM state into host coordinates. */
    if (demo->status != FVM_OK) /* Never use uninitialized pointer coordinates. */
        return demo->status; /* Caller performs normal cleanup. */
    if (x < -DEMO_MARKER_HALF) x = -DEMO_MARKER_HALF; /* Bound drawing only, preserve raw VM input. */
    if (y < -DEMO_MARKER_HALF) y = -DEMO_MARKER_HALF; /* Avoid raster coordinate limits offstage. */
    if (x > DEMO_WIDTH + DEMO_MARKER_HALF) x = DEMO_WIDTH + DEMO_MARKER_HALF; /* Clip invisible marker. */
    if (y > DEMO_HEIGHT + DEMO_MARKER_HALF) y = DEMO_HEIGHT + DEMO_MARKER_HALF; /* Keep geometry bounded. */
    for (offset = 0; offset < demo->byte_count; offset += FVM_HOST_RGBA_BYTES) /* Fill opaque RGBA. */
    { /* Keep background independent of previous frame contents. */
        memset(demo->frame.pixels + offset, DEMO_IDLE_RGB, DEMO_RGB_CHANNELS); /* Equal RGB channels form dark gray. */
        demo->frame.pixels[offset + DEMO_RGB_CHANNELS] = DEMO_OPAQUE; /* Opaque alpha avoids backend blend ambiguity. */
    } /* Every accessible pixel is initialized before presentation. */
    if (demo->held) /* Use red while the primary button/contact is held. */
    { /* Change the color without changing the underlying input contract. */
        color.r = DEMO_HELD_RED; /* Held marker red channel. */
        color.g = DEMO_HELD_GREEN; /* Held marker green channel. */
    } /* Blue/alpha retain their initialized values. */
    points[0].x = points[3].x = x - DEMO_MARKER_HALF; /* Left edge in world pixels. */
    points[1].x = points[2].x = x + DEMO_MARKER_HALF; /* Right edge in world pixels. */
    points[0].y = points[1].y = y - DEMO_MARKER_HALF; /* Top edge. */
    points[2].y = points[3].y = y + DEMO_MARKER_HALF; /* Bottom edge. */
    demo->status = fvm_raster_fill_polygon(&demo->frame, points, DEMO_CORNERS, color); /* Clip polygon to image. */
    if (demo->status == FVM_OK) /* Do not present an unsuccessful raster result. */
        demo->status = fvm_host_present(&demo->frame, demo->byte_count, present, userdata); /* Borrow span. */
    if (demo->status == FVM_OK) /* Count frames only after backend success. */
        ++demo->frames; /* Used by bounded smoke runs. */
    return demo->status; /* NO_GRAPHICS is an actual unsupported status, never fake rendering. */
} /* The consumer has finished with the borrowed frame before return. */
#endif /* End this executable-private C89 tutorial implementation. */
