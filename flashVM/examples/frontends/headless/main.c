/* A synchronous headless frame consumer; see HOST_BACKENDS.md for replacing it with a GPU upload. */
#include "flashvm/host.h"  /* Borrow RGBA frames and invert event-space transforms. */
#include "flashvm/input.h" /* Query the independent VM pointer state after dispatch. */
#include "flashvm/mouse.h" /* Deliver primary-contact events without SDL. */
#include <stdio.h>         /* Report frame consumption, mapped input and ordinary failures. */
#include <string.h>        /* Initialize host-owned pixel storage before rasterization. */
#define HOST_DEMO_SIZE 16U /* Small logical square, independent of any window dimensions. */


/* Own the consumer counter in main until every synchronous call has returned. */
static FVM_Result consume_frame(const FVM_RasterSurface *frame, void *userdata)         /* Borrow frame/userdata. */
{                                                                                       /* Read pixels only within this callback lifetime. */
    unsigned int *frames = (unsigned int *)userdata;                                    /* Main owns this counter. */
    const unsigned char *pixel = frame->pixels + frame->stride * (HOST_DEMO_SIZE / 2) + /* Select row. */
                                 FVM_HOST_RGBA_BYTES * (HOST_DEMO_SIZE / 2);            /* Select center column. */
    if (pixel[0] != 255 || pixel[1] || pixel[2] || pixel[3] != 255)                     /* Require opaque red RGBA. */
    {                                                                                   /* Begin conditional branch: Require opaque red RGBA. */
        return FVM_ERROR;                                                               /* A failed consumer remains a real host error. */
    } /* End conditional branch: Require opaque red RGBA. */
    ++*frames;                                                                                 /* Count only a fully inspected frame. */
    printf("frame %u: %ux%u RGBA center=255,0,0,255\n", *frames, frame->width, frame->height); /* No retained pixels. */
    return FVM_OK;                                                                             /* A GPU adapter would finish upload/copy before this return. */
} /* All borrowed frame pointers expire here. */


/* Build a real raster frame and deliver a host-space contact on one VM owning thread. */
int main(void)                                                                   /* No native window or external display SDK is required. */
{                                                                                /* Acquire host storage and VM owners in dependency order. */
    unsigned char pixels[HOST_DEMO_SIZE * HOST_DEMO_SIZE * FVM_HOST_RGBA_BYTES]; /* Host owns bytes. */
    FVM_RasterSurface frame = {0, HOST_DEMO_SIZE, HOST_DEMO_SIZE,                /* Logical extent. */
                               HOST_DEMO_SIZE * FVM_HOST_RGBA_BYTES};            /* Tightly packed stride. */
    FVM_RasterPoint corners[4] = {{2, 2}, {14, 2}, {14, 14}, {2, 14}};           /* Interior red square. */
    FVM_RasterColor red = {255, 0, 0, 255};                                      /* Straight-alpha opaque fill. */
    FVM_Affine world_to_host = {2, 0, 0, 2, 100, 50};                            /* Device scale and actor offset, once. */
    FVM_Config config;                                                           /* Own initialized configuration storage. */
    FVM_Runtime *runtime = 0;                                                    /* Destroy after the context. */
    FVM_Context *context = 0;                                                    /* Own pointer state without any platform types. */
    unsigned int frames = 0;                                                     /* Borrow to the synchronous frame consumer only. */
    double world_x = 0, world_y = 0;                                             /* Commit only successful coordinate conversion. */
    double seen_x = 0, seen_y = 0;                                               /* Read back the context's independent pointer state. */
    FVM_Result status;                                                           /* Preserve the first real failure for diagnostics. */
    frame.pixels = pixels;                                                       /* Assign automatic storage after C89 constant initialization. */
    memset(pixels, 0, sizeof(pixels));                                           /* Initialize transparent background, including alpha. */
    status = fvm_raster_fill_polygon(&frame, corners, 4, red);                   /* Requires GRAPHICS_AGG or GRAPHICS_CAIRO. */
    if (status == FVM_UNSUPPORTED)                                               /* Explicitly skip when no raster backend is built. */
    {                                                                            /* Begin conditional branch: Explicitly skip when no raster backend is built. */
        return 77;                                                               /* No VM owner exists yet; this is not successful pixel acceptance. */
    } /* End conditional branch: Explicitly skip when no raster backend is built. */
    if (status == FVM_OK)                                                          /* Do not present an unsuccessful or unsupported raster result. */
    {                                                                              /* Begin conditional branch: Do not present an unsuccessful or unsupported raster result. */
        status = fvm_host_present(&frame, sizeof(pixels), consume_frame, &frames); /* Borrow actual allocation span. */
    } /* End conditional branch: Do not present an unsuccessful or unsupported raster result. */
    if (status == FVM_OK)                  /* Prepare VM configuration only after consuming the frame. */
    {                                      /* Begin conditional branch: Prepare VM configuration only after consuming the frame. */
        status = fvm_config_init(&config); /* Initialize supported ABI fields. */
    } /* End conditional branch: Prepare VM configuration only after consuming the frame. */
    if (status == FVM_OK)                               /* Own runtime before creating its context. */
    {                                                   /* Begin conditional branch: Own runtime before creating its context. */
        status = fvm_runtime_create(&config, &runtime); /* No SDL/window initialization. */
    } /* End conditional branch: Own runtime before creating its context. */
    if (status == FVM_OK)                               /* Establish a single-thread VM input owner. */
    {                                                   /* Begin conditional branch: Establish a single-thread VM input owner. */
        status = fvm_context_create(runtime, &context); /* No script listeners are needed for pointer publication. */
    } /* End conditional branch: Establish a single-thread VM input owner. */
    if (status == FVM_OK)                                                                /* Convert a host event using exactly the presentation transform. */
    {                                                                                    /* Begin conditional branch: Convert a host event using exactly the presentation transform. */
        status = fvm_host_pointer_to_world(&world_to_host, 120, 90, &world_x, &world_y); /* Expect 10,20. */
    } /* End conditional branch: Convert a host event using exactly the presentation transform. */
    if (status == FVM_OK)                                                                   /* A real adapter selects the primary contact before this call. */
    {                                                                                       /* Begin conditional branch: A real adapter selects the primary contact before this call. */
        status = fvm_input_dispatch_mouse(context, FVM_MOUSE_EVENT_DOWN, world_x, world_y); /* Owner-thread dispatch. */
    } /* End conditional branch: A real adapter selects the primary contact before this call. */
    if (status == FVM_OK)                                                                 /* End the same contact without a synthetic duplicate mouse event. */
    {                                                                                     /* Begin conditional branch: End the same contact without a synthetic duplicate mouse event. */
        status = fvm_input_dispatch_mouse(context, FVM_MOUSE_EVENT_UP, world_x, world_y); /* Publish final world position. */
    } /* End conditional branch: End the same contact without a synthetic duplicate mouse event. */
    if (status == FVM_OK)                                          /* Verify that the VM received world coordinates rather than host coordinates. */
    {                                                              /* Begin conditional branch: Verify that the VM received world coordinates rather than host coordinates. */
        status = fvm_input_get_pointer(context, &seen_x, &seen_y); /* Copy values without retaining VM storage. */
    } /* End conditional branch: Verify that the VM received world coordinates rather than host coordinates. */
    if (status == FVM_OK && (frames != 1 || seen_x != 10 || seen_y != 20)) /* Normal tutorial error handling. */
    {                                                                      /* Begin conditional branch: Normal tutorial error handling. */
        status = FVM_ERROR;                                                /* Report unexpected contract behavior without assertions. */
    } /* End conditional branch: Normal tutorial error handling. */
    if (status == FVM_OK)                                    /* Report the fully completed headless demonstration. */
    {                                                        /* Begin conditional branch: Report the fully completed headless demonstration. */
        printf("pointer world=%.0f,%.0f\n", seen_x, seen_y); /* Expect pointer world=10,20. */
    } /* End conditional branch: Report the fully completed headless demonstration. */
    if (context)                      /* Destroy only an acquired context. */
    {                                 /* Begin conditional branch: Destroy only an acquired context. */
        fvm_context_destroy(context); /* Release input/managed state while runtime is still live. */
    } /* End conditional branch: Destroy only an acquired context. */
    if (runtime)                      /* Release the outer owner last. */
    {                                 /* Begin conditional branch: Release the outer owner last. */
        fvm_runtime_destroy(runtime); /* Host pixels and callback userdata remain stack-owned. */
    } /* End conditional branch: Release the outer owner last. */
    if (status != FVM_OK)                                           /* Distinguish unsupported graphics from a successful demonstration. */
    {                                                               /* Begin conditional branch: Distinguish unsupported graphics from a successful demonstration. */
        fprintf(stderr, "headless host failed: %d\n", (int)status); /* Return the original status for diagnosis. */
    } /* End conditional branch: Distinguish unsupported graphics from a successful demonstration. */
    return status == FVM_OK ? 0 : 1; /* All synchronous callbacks have finished before stack release. */
} /* No frame, window, texture or VM pointer escapes main. */
