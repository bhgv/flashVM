/* Native wheel normalization boundary shared by the annotated frontend hosts. */
#ifndef FVM_FRONTEND_WHEEL_H /* Keep this executable-private helper independent of window SDKs. */
#define FVM_FRONTEND_WHEEL_H /* No GUI/kernel types enter this portable wheel wrapper. */
#include "demo.h" /* Borrow Demo ownership, status latch and world-coordinate conversion. */
#define DEMO_WHEEL_LINES_PER_DETENT 3.0 /* Explicit tutorial policy, not an OS preference query. */
/* Convert native event position once and deliver already normalized signed scroll lines. */
static FVM_Result demo_wheel(Demo *demo, const FVM_Affine *world_to_event, /* Borrow forward transform. */
                             double x, double y, double delta) /* Event pixels and host-normalized lines. */
{ /* This is synchronous; no native event pointer or asynchronous frame is retained. */
    double world_x, world_y; /* Initialized only by successful inverse conversion. */
    if (demo->status != FVM_OK) return demo->status; /* Preserve the first host/VM failure. */
    demo->status = fvm_host_pointer_to_world(world_to_event, x, y, &world_x, &world_y); /* Match painting. */
    if (demo->status == FVM_OK) /* Do not publish a position if the native transform is unusable. */
        demo->status = fvm_input_dispatch_mouse_wheel(demo->context, delta, world_x, world_y); /* Rooted delivery. */
    if (demo->status == FVM_OK) { /* Count only events accepted by the actual VM dispatcher. */
        ++demo->wheel_events; /* Not a count of raw OS packets or physical wheel detents. */
        demo->last_wheel_delta = delta; /* Keep fraction/sign, never accumulate or silently quantize. */
    } /* Button hold state is deliberately unchanged by wheel input. */
    return demo->status; /* Native loops stop on mapping, allocation or handler errors. */
} /* Demo and borrowed transform remain owned by their caller. */
#endif /* End the shared owner-thread wheel boundary. */
