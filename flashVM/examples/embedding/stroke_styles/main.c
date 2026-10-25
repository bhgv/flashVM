/* C89 tutorial: selected caps and round, linear and adaptive curve bounds; see README.md. */
#include "flashvm/standard.h"  /* Declare runtime setup, values and ordinary object calls. */
#include "flashvm/movieclip.h" /* Declare host MovieClip creation and synchronous rendering. */
#include "flashvm/gc.h"        /* [GC:] Declare explicit roots owned by this host. */
#include <stdio.h>             /* Report ordinary errors and the verified geometry. */
#include <string.h>            /* Initialize the host-owned RGBA buffer. */
#define STYLE_WIDTH 16U        /* Destination width in pixels. */
#define STYLE_HEIGHT 16U       /* Destination height in pixels. */
#define STYLE_RGBA 4U          /* Straight RGBA bytes per pixel. */


/* Report an API error; no assertion or process-global recovery is required. */
static int checked(FVM_Result status, const char *operation) /* Borrow the diagnostic label. */
{                                                            /* Begin status checking. */
    if (status == FVM_OK)                                    /* Condition for this action: Continue only after a successful synchronous call. */
    {                                                        /* Begin conditional branch: Condition for this action: Continue only after a successful synchronous call. */
        return 1;                                            /* Continue only after a successful synchronous call. */
    } /* End conditional branch: Condition for this action: Continue only after a successful synchronous call. */
    fprintf(stderr, "%s: %d\n", operation, (int)status); /* Keep the native status visible. */
    return 0;                                            /* Ask main to release its roots and context. */
} /* End status checking. */


/* Read one scalar while the caller keeps the result object pinned. */
static int scalar(FVM_Context *context, FVM_Object *object, const char *key, double *number) /* Borrow inputs. */
{                                                                                            /* Begin scalar reading. */
    FVM_Value value;                                                                         /* Receive a copied value whose numeric member owns no resource. */
    if (!checked(fvm_object_get(context, object, key, &value), key))                         /* Condition for this action: Read through the public API. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Read through the public API. */
        return 0;                                                                            /* Read through the public API. */
    } /* End conditional branch: Condition for this action: Read through the public API. */
    if (value.type != FVM_VALUE_NUMBER) /* Condition for this action: Reject unexpected result representations. */
    {                                   /* Begin conditional branch: Condition for this action: Reject unexpected result representations. */
        return 0;                       /* Reject unexpected result representations. */
    } /* End conditional branch: Condition for this action: Reject unexpected result representations. */
    *number = value.number; /* Copy the scalar before the next VM call. */
    return 1;               /* The borrowed object remains owned and rooted by main. */
} /* End scalar reading. */


/* Create a host-authored clip; an unloaded context exposes the latest supported script options. */
int main(void)                                                     /* Run without an external SWF compiler or device event loop. */
{                                                                  /* Begin the executable tutorial. */
    FVM_Config config;                                             /* Store caller-owned runtime configuration. */
    FVM_Runtime *runtime = 0;                                      /* Own the runtime until the final cleanup step. */
    FVM_Context *context = 0;                                      /* Own one isolated execution and collection context. */
    FVM_Object *clip = 0, *bounds = 0;                             /* Borrow context-owned objects while explicitly pinned. */
    FVM_Value arguments[8], out;                                   /* Hold synchronous operands and copied results. */
    unsigned char pixels[STYLE_WIDTH * STYLE_HEIGHT * STYLE_RGBA]; /* Own destination bytes on the stack. */
    FVM_RasterSurface surface;                                     /* Initialize borrowed surface fields below in strict C89 form. */
    FVM_Result rendered;                                           /* Preserve renderer availability independently of geometric success. */
    double left = 0, right = 0, top = 0, bottom = 0;               /* Copy bounds before releasing their temporary object. */
    unsigned int i;                                                /* Traverse the fixed argument array in C89 mode. */
    int clip_root = 0, bounds_root = 0, result = 1;                /* Track exactly which roots cleanup owns. */
    surface.pixels = pixels;                                       /* Borrow the host-owned RGBA storage. */
    surface.width = STYLE_WIDTH;                                   /* Set destination width without a C99 aggregate initializer. */
    surface.height = STYLE_HEIGHT;                                 /* Set destination height in pixels. */
    surface.stride = STYLE_WIDTH * STYLE_RGBA;                     /* Set one row stride in bytes. */
    if (!checked(fvm_config_init(&config), "config"))              /* Condition for this action: Initialize all configuration fields. */
    {                                                              /* Begin conditional branch: Condition for this action: Initialize all configuration fields. */
        goto done;                                                 /* Initialize all configuration fields. */
    } /* End conditional branch: Condition for this action: Initialize all configuration fields. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Condition for this action: Create the host-owned runtime. */
    {                                                               /* Begin conditional branch: Condition for this action: Create the host-owned runtime. */
        goto done;                                                  /* Create the host-owned runtime. */
    } /* End conditional branch: Condition for this action: Create the host-owned runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Condition for this action: Create the isolated VM. */
    {                                                               /* Begin conditional branch: Condition for this action: Create the isolated VM. */
        goto done;                                                  /* Create the isolated VM. */
    } /* End conditional branch: Condition for this action: Create the isolated VM. */
    if (!checked(fvm_standard_install(context), "standard")) /* Condition for this action: Install MovieClip script methods. */
    {                                                        /* Begin conditional branch: Condition for this action: Install MovieClip script methods. */
        goto done;                                           /* Install MovieClip script methods. */
    } /* End conditional branch: Condition for this action: Install MovieClip script methods. */
    if (!checked(fvm_movieclip_create(context, "stroke", 1, &clip), "clip")) /* Condition for this action: Create context-owned graphics. */
    {                                                                        /* Begin conditional branch: Condition for this action: Create context-owned graphics. */
        goto done;                                                           /* Create context-owned graphics. */
    } /* End conditional branch: Condition for this action: Create context-owned graphics. */
    if (!checked(fvm_gc_pin(context, clip), "pin clip")) /* Condition for this action: [GC:] Retain the clip through every VM call. */
    {                                                    /* Begin conditional branch: Condition for this action: [GC:] Retain the clip through every VM call. */
        goto done;                                       /* [GC:] Retain the clip through every VM call. */
    } /* End conditional branch: Condition for this action: [GC:] Retain the clip through every VM call. */
    clip_root = 1;                          /* Record the root only after successful acquisition. */
    for (i = 0; i < 8; ++i)                 /* Default all optional parameters explicitly. */
    {                                       /* Begin loop body: Default all optional parameters explicitly. */
        fvm_value_undefined(arguments + i); /* Default all optional parameters explicitly. */
    } /* End loop body: Default all optional parameters explicitly. */
    fvm_value_number(arguments, 4);                                   /* Choose a four-pixel stroke. */
    fvm_value_number(arguments + 1, 0xff0000);                        /* Choose opaque red RGB. */
    fvm_value_number(arguments + 2, 100);                             /* Use one hundred percent opacity. */
    fvm_value_boolean(arguments + 3, 0);                              /* Keep unsupported pixel hinting disabled. */
    if (!checked(fvm_value_string(arguments + 4, "normal"), "scale")) /* Condition for this action: Borrow a static scale-mode string. */
    {                                                                 /* Begin conditional branch: Condition for this action: Borrow a static scale-mode string. */
        goto done;                                                    /* Borrow a static scale-mode string. */
    } /* End conditional branch: Condition for this action: Borrow a static scale-mode string. */
    if (!checked(fvm_value_string(arguments + 5, "square"), "cap")) /* Condition for this action: Select symmetric square ends. */
    {                                                               /* Begin conditional branch: Condition for this action: Select symmetric square ends. */
        goto done;                                                  /* Select symmetric square ends. */
    } /* End conditional branch: Condition for this action: Select symmetric square ends. */
    if (!checked(fvm_value_string(arguments + 6, "bevel"), "join")) /* Condition for this action: Select bevel joins for this path. */
    {                                                               /* Begin conditional branch: Condition for this action: Select bevel joins for this path. */
        goto done;                                                  /* Select bevel joins for this path. */
    } /* End conditional branch: Condition for this action: Select bevel joins for this path. */
    if (!checked(fvm_object_call(context, clip, "lineStyle", 7, arguments, &out), "lineStyle")) /* Condition for this action: Copy the current style atomically. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Copy the current style atomically. */
        goto done;                                                                              /* Copy the current style atomically. */
    } /* End conditional branch: Condition for this action: Copy the current style atomically. */
    fvm_value_number(arguments, 2);                                                       /* Begin at horizontal coordinate two. */
    fvm_value_number(arguments + 1, 8);                                                   /* Center the line vertically. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "moveTo")) /* Condition for this action: Start a path retaining the style. */
    {                                                                                     /* Begin conditional branch: Condition for this action: Start a path retaining the style. */
        goto done;                                                                        /* Start a path retaining the style. */
    } /* End conditional branch: Condition for this action: Start a path retaining the style. */
    fvm_value_number(arguments, 14);                                                      /* End twelve pixels to the right. */
    if (!checked(fvm_object_call(context, clip, "lineTo", 2, arguments, &out), "lineTo")) /* Condition for this action: Append the segment without running a timeline. */
    {                                                                                     /* Begin conditional branch: Condition for this action: Append the segment without running a timeline. */
        goto done;                                                                        /* Append the segment without running a timeline. */
    } /* End conditional branch: Condition for this action: Append the segment without running a timeline. */
    fvm_value_undefined(arguments);                                                             /* Initialize the object-valued reference-space operand. */
    arguments[0].type = FVM_VALUE_OBJECT;                                                       /* Mark the following pointer as a managed object reference. */
    arguments[0].object = clip;                                                                 /* Borrow the already pinned clip as the bounds coordinate space. */
    if (!checked(fvm_object_call(context, clip, "getBounds", 1, arguments, &out), "getBounds")) /* Condition for this action: Allocate a context-owned result object. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Allocate a context-owned result object. */
        goto done;                                                                              /* Allocate a context-owned result object. */
    } /* End conditional branch: Condition for this action: Allocate a context-owned result object. */
    if (out.type != FVM_VALUE_OBJECT || !out.object) /* Condition for this action: Validate before borrowing its pointer. */
    {                                                /* Begin conditional branch: Condition for this action: Validate before borrowing its pointer. */
        goto done;                                   /* Validate before borrowing its pointer. */
    } /* End conditional branch: Condition for this action: Validate before borrowing its pointer. */
    bounds = out.object;                                     /* Keep the returned object separate from later scalar results. */
    if (!checked(fvm_gc_pin(context, bounds), "pin bounds")) /* Condition for this action: [GC:] Retain the temporary result across property reads. */
    {                                                        /* Begin conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
        goto done;                                           /* [GC:] Retain the temporary result across property reads. */
    } /* End conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
    bounds_root = 1;                                                                         /* [GC:] Record ownership of the second explicit root. */
    if (!scalar(context, bounds, "xMin", &left) || !scalar(context, bounds, "xMax", &right)) /* Condition for this action: Copy the horizontal extents. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy the horizontal extents. */
        goto done;                                                                           /* Copy the horizontal extents. */
    } /* End conditional branch: Condition for this action: Copy the horizontal extents. */
    if (left != 0 || right != 16) /* Condition for this action: Square caps extend by half the stroke width at each end. */
    {                             /* Begin conditional branch: Condition for this action: Square caps extend by half the stroke width at each end. */
        goto done;                /* Square caps extend by half the stroke width at each end. */
    } /* End conditional branch: Condition for this action: Square caps extend by half the stroke width at each end. */
    fvm_value_number(arguments, 0.1);                                                       /* Probe a square-only corner outside the old round cap. */
    fvm_value_number(arguments + 1, 6.1);                                                   /* Stay strictly inside the top-left corner. */
    fvm_value_boolean(arguments + 2, 1);                                                    /* Request geometry rather than only a bounding box. */
    if (!checked(fvm_object_call(context, clip, "hitTest", 3, arguments, &out), "hitTest")) /* Condition for this action: Query selected cap geometry. */
    {                                                                                       /* Begin conditional branch: Condition for this action: Query selected cap geometry. */
        goto done;                                                                          /* Query selected cap geometry. */
    } /* End conditional branch: Condition for this action: Query selected cap geometry. */
    if (out.type != FVM_VALUE_BOOLEAN || !out.boolean) /* Condition for this action: Require the square corner to be covered. */
    {                                                  /* Begin conditional branch: Condition for this action: Require the square corner to be covered. */
        goto done;                                     /* Require the square corner to be covered. */
    } /* End conditional branch: Condition for this action: Require the square corner to be covered. */
    printf("bounds: %.0f..%.0f; square corner: hit\n", left, right); /* Report backend-independent results. */
    memset(pixels, 0, sizeof(pixels));                               /* Prepare transparent caller-owned destination pixels. */
    rendered = fvm_movieclip_render(context, clip, &surface);        /* Borrow the buffer for one synchronous render. */
    if (rendered == FVM_UNSUPPORTED)                                 /* Condition for this action: Keep SDK-free execution useful. */
    {                                                                /* Begin conditional branch: Condition for this action: Keep SDK-free execution useful. */
        puts("renderer unavailable; geometry verified");             /* Keep SDK-free execution useful. */
    } /* End conditional branch: Condition for this action: Keep SDK-free execution useful. */
    else if (!checked(rendered, "render")) /* Treat real renderer failures as errors. */
    {                                      /* Begin conditional branch: Treat real renderer failures as errors. */
        goto done;                         /* Treat real renderer failures as errors. */
    } /* End conditional branch: Treat real renderer failures as errors. */
    else if (pixels[(6 * STYLE_WIDTH) * STYLE_RGBA + 3] != 255) /* Require the fully covered square-cap corner. */
    {                                                           /* Begin conditional branch: Require the fully covered square-cap corner. */
        goto done;                                              /* Require the fully covered square-cap corner. */
    } /* End conditional branch: Require the fully covered square-cap corner. */
    else                                  /* Report the independently checked raster sample. */
    {                                     /* Begin conditional branch: Report the independently checked raster sample. */
        puts("square corner alpha: 255"); /* Report the independently checked raster sample. */
    } /* End conditional branch: Report the independently checked raster sample. */
    if (!checked(fvm_gc_unpin(context, bounds), "unpin old bounds")) /* Condition for this action: [GC:] Release the first result before replacing geometry. */
    {                                                                /* Begin conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
        goto done;                                                   /* [GC:] Release the first result before replacing geometry. */
    } /* End conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
    bounds_root = 0;                                                            /* [GC:] Cleanup no longer owns the first bounds root. */
    bounds = 0;                                                                 /* [GC:] Do not reuse the unrooted borrowed pointer. */
    if (!checked(fvm_object_call(context, clip, "clear", 0, 0, &out), "clear")) /* Condition for this action: Remove the square stroke before the independent curve query. */
    {                                                                           /* Begin conditional branch: Condition for this action: Remove the square stroke before the independent curve query. */
        goto done;                                                              /* Remove the square stroke before the independent curve query. */
    } /* End conditional branch: Condition for this action: Remove the square stroke before the independent curve query. */
    for (i = 0; i < 8; ++i)                 /* Default all optional parameters explicitly. */
    {                                       /* Begin loop body: Default all optional parameters explicitly. */
        fvm_value_undefined(arguments + i); /* Default all optional parameters explicitly. */
    } /* End loop body: Default all optional parameters explicitly. */
    fvm_value_number(arguments, 4);                                   /* Choose a four-pixel stroke. */
    fvm_value_number(arguments + 1, 0xff0000);                        /* Choose opaque red RGB. */
    fvm_value_number(arguments + 2, 100);                             /* Use one hundred percent opacity. */
    fvm_value_boolean(arguments + 3, 0);                              /* Keep unsupported pixel hinting disabled. */
    if (!checked(fvm_value_string(arguments + 4, "normal"), "scale")) /* Condition for this action: Borrow a static scale-mode string. */
    {                                                                 /* Begin conditional branch: Condition for this action: Borrow a static scale-mode string. */
        goto done;                                                    /* Borrow a static scale-mode string. */
    } /* End conditional branch: Condition for this action: Borrow a static scale-mode string. */
    if (!checked(fvm_value_string(arguments + 5, "round"), "cap")) /* Condition for this action: Select symmetric round ends. */
    {                                                              /* Begin conditional branch: Condition for this action: Select symmetric round ends. */
        goto done;                                                 /* Select symmetric round ends. */
    } /* End conditional branch: Condition for this action: Select symmetric round ends. */
    if (!checked(fvm_value_string(arguments + 6, "round"), "join")) /* Condition for this action: Select round joins for this path. */
    {                                                               /* Begin conditional branch: Condition for this action: Select round joins for this path. */
        goto done;                                                  /* Select round joins for this path. */
    } /* End conditional branch: Condition for this action: Select round joins for this path. */
    if (!checked(fvm_object_call(context, clip, "lineStyle", 7, arguments, &out), "lineStyle")) /* Condition for this action: Copy the current style atomically. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Copy the current style atomically. */
        goto done;                                                                              /* Copy the current style atomically. */
    } /* End conditional branch: Condition for this action: Copy the current style atomically. */
    fvm_value_number(arguments, 2);                                                            /* Start the round curve at x=2. */
    fvm_value_number(arguments + 1, 8);                                                        /* Start at y=8. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "curve start")) /* Condition for this action: Begin a separate round path. */
    {                                                                                          /* Begin conditional branch: Condition for this action: Begin a separate round path. */
        goto done;                                                                             /* Begin a separate round path. */
    } /* End conditional branch: Condition for this action: Begin a separate round path. */
    fvm_value_number(arguments, 8);                                                         /* Place the quadratic control at x=8. */
    fvm_value_number(arguments + 1, 0);                                                     /* Place the control at y=0, above the actual curve. */
    fvm_value_number(arguments + 2, 14);                                                    /* End the quadratic at x=14. */
    fvm_value_number(arguments + 3, 8);                                                     /* End at y=8; the curve minimum is y=4. */
    if (!checked(fvm_object_call(context, clip, "curveTo", 4, arguments, &out), "curveTo")) /* Condition for this action: Copy the quadratic into the retained path. */
    {                                                                                       /* Begin conditional branch: Condition for this action: Copy the quadratic into the retained path. */
        goto done;                                                                          /* Copy the quadratic into the retained path. */
    } /* End conditional branch: Condition for this action: Copy the quadratic into the retained path. */
    fvm_value_number(arguments, 100);                                                            /* Choose a distant isolated move. */
    fvm_value_number(arguments + 1, 100);                                                        /* Keep the isolated point outside the actual stroke. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "isolated move")) /* Condition for this action: A move without a segment must not enlarge stroke bounds. */
    {                                                                                            /* Begin conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
        goto done;                                                                               /* A move without a segment must not enlarge stroke bounds. */
    } /* End conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
    fvm_value_undefined(arguments);                                                             /* Initialize the object-valued reference-space operand. */
    arguments[0].type = FVM_VALUE_OBJECT;                                                       /* Mark the following pointer as a managed object reference. */
    arguments[0].object = clip;                                                                 /* Borrow the already pinned clip as the bounds coordinate space. */
    if (!checked(fvm_object_call(context, clip, "getBounds", 1, arguments, &out), "getBounds")) /* Condition for this action: Allocate a context-owned result object. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Allocate a context-owned result object. */
        goto done;                                                                              /* Allocate a context-owned result object. */
    } /* End conditional branch: Condition for this action: Allocate a context-owned result object. */
    if (out.type != FVM_VALUE_OBJECT || !out.object) /* Condition for this action: Validate before borrowing its pointer. */
    {                                                /* Begin conditional branch: Condition for this action: Validate before borrowing its pointer. */
        goto done;                                   /* Validate before borrowing its pointer. */
    } /* End conditional branch: Condition for this action: Validate before borrowing its pointer. */
    bounds = out.object;                                     /* Keep the returned object separate from later scalar results. */
    if (!checked(fvm_gc_pin(context, bounds), "pin bounds")) /* Condition for this action: [GC:] Retain the temporary result across property reads. */
    {                                                        /* Begin conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
        goto done;                                           /* [GC:] Retain the temporary result across property reads. */
    } /* End conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
    bounds_root = 1;                                                                         /* [GC:] Record ownership of the second explicit root. */
    if (!scalar(context, bounds, "xMin", &left) || !scalar(context, bounds, "xMax", &right)) /* Condition for this action: Copy the horizontal extents. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy the horizontal extents. */
        goto done;                                                                           /* Copy the horizontal extents. */
    } /* End conditional branch: Condition for this action: Copy the horizontal extents. */
    if (!scalar(context, bounds, "yMin", &top) || !scalar(context, bounds, "yMax", &bottom)) /* Condition for this action: Copy vertical analytic extrema. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy vertical analytic extrema. */
        goto done;                                                                           /* Copy vertical analytic extrema. */
    } /* End conditional branch: Condition for this action: Copy vertical analytic extrema. */
    if (left != 0 || right != 16 || top != 2 || bottom != 10) /* Condition for this action: Expand the curve extrema by the two-unit round radius. */
    {                                                         /* Begin conditional branch: Condition for this action: Expand the curve extrema by the two-unit round radius. */
        goto done;                                            /* Expand the curve extrema by the two-unit round radius. */
    } /* End conditional branch: Condition for this action: Expand the curve extrema by the two-unit round radius. */
    printf("round curve bounds: %.0f..%.0f, %.0f..%.0f\n", left, right, top, bottom); /* Report the move-independent geometric result. */
    if (!checked(fvm_gc_unpin(context, bounds), "unpin old bounds"))                  /* Condition for this action: [GC:] Release the first result before replacing geometry. */
    {                                                                                 /* Begin conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
        goto done;                                                                    /* [GC:] Release the first result before replacing geometry. */
    } /* End conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
    bounds_root = 0;                                                            /* [GC:] Cleanup no longer owns the first bounds root. */
    bounds = 0;                                                                 /* [GC:] Do not reuse the unrooted borrowed pointer. */
    if (!checked(fvm_object_call(context, clip, "clear", 0, 0, &out), "clear")) /* Condition for this action: Remove the preceding stroke before the independent axis query. */
    {                                                                           /* Begin conditional branch: Condition for this action: Remove the preceding stroke before the independent axis query. */
        goto done;                                                              /* Remove the preceding stroke before the independent axis query. */
    } /* End conditional branch: Condition for this action: Remove the preceding stroke before the independent axis query. */
    for (i = 0; i < 8; ++i)                 /* Default all optional parameters explicitly. */
    {                                       /* Begin loop body: Default all optional parameters explicitly. */
        fvm_value_undefined(arguments + i); /* Default all optional parameters explicitly. */
    } /* End loop body: Default all optional parameters explicitly. */
    fvm_value_number(arguments, 4);                                   /* Choose a four-pixel stroke. */
    fvm_value_number(arguments + 1, 0xff0000);                        /* Choose opaque red RGB. */
    fvm_value_number(arguments + 2, 100);                             /* Use one hundred percent opacity. */
    fvm_value_boolean(arguments + 3, 0);                              /* Keep unsupported pixel hinting disabled. */
    if (!checked(fvm_value_string(arguments + 4, "normal"), "scale")) /* Condition for this action: Borrow a static scale-mode string. */
    {                                                                 /* Begin conditional branch: Condition for this action: Borrow a static scale-mode string. */
        goto done;                                                    /* Borrow a static scale-mode string. */
    } /* End conditional branch: Condition for this action: Borrow a static scale-mode string. */
    if (!checked(fvm_value_string(arguments + 5, "none"), "cap")) /* Condition for this action: Select symmetric butt ends. */
    {                                                             /* Begin conditional branch: Condition for this action: Select symmetric butt ends. */
        goto done;                                                /* Select symmetric butt ends. */
    } /* End conditional branch: Condition for this action: Select symmetric butt ends. */
    if (!checked(fvm_value_string(arguments + 6, "bevel"), "join")) /* Condition for this action: Select bevel joins for this path. */
    {                                                               /* Begin conditional branch: Condition for this action: Select bevel joins for this path. */
        goto done;                                                  /* Select bevel joins for this path. */
    } /* End conditional branch: Condition for this action: Select bevel joins for this path. */
    if (!checked(fvm_object_call(context, clip, "lineStyle", 7, arguments, &out), "lineStyle")) /* Condition for this action: Copy the current style atomically. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Copy the current style atomically. */
        goto done;                                                                              /* Copy the current style atomically. */
    } /* End conditional branch: Condition for this action: Copy the current style atomically. */
    fvm_value_number(arguments, 2);                                                            /* Start the axis curve at x=2. */
    fvm_value_number(arguments + 1, 8);                                                        /* Start at y=8. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "curve start")) /* Condition for this action: Begin a separate butt/bevel path. */
    {                                                                                          /* Begin conditional branch: Condition for this action: Begin a separate butt/bevel path. */
        goto done;                                                                             /* Begin a separate butt/bevel path. */
    } /* End conditional branch: Condition for this action: Begin a separate butt/bevel path. */
    fvm_value_number(arguments, 8);                                                         /* Place the quadratic control at x=8. */
    fvm_value_number(arguments + 1, 8);                                                     /* Keep the control on the horizontal segment. */
    fvm_value_number(arguments + 2, 14);                                                    /* End the quadratic at x=14. */
    fvm_value_number(arguments + 3, 8);                                                     /* End at y=8; all controls progress without reversal. */
    if (!checked(fvm_object_call(context, clip, "curveTo", 4, arguments, &out), "curveTo")) /* Condition for this action: Copy the quadratic into the retained path. */
    {                                                                                       /* Begin conditional branch: Condition for this action: Copy the quadratic into the retained path. */
        goto done;                                                                          /* Copy the quadratic into the retained path. */
    } /* End conditional branch: Condition for this action: Copy the quadratic into the retained path. */
    fvm_value_number(arguments, 100);                                                            /* Choose a distant isolated move. */
    fvm_value_number(arguments + 1, 100);                                                        /* Keep the isolated point outside the actual stroke. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "isolated move")) /* Condition for this action: A move without a segment must not enlarge stroke bounds. */
    {                                                                                            /* Begin conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
        goto done;                                                                               /* A move without a segment must not enlarge stroke bounds. */
    } /* End conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
    fvm_value_undefined(arguments);                                                             /* Initialize the object-valued reference-space operand. */
    arguments[0].type = FVM_VALUE_OBJECT;                                                       /* Mark the following pointer as a managed object reference. */
    arguments[0].object = clip;                                                                 /* Borrow the already pinned clip as the bounds coordinate space. */
    if (!checked(fvm_object_call(context, clip, "getBounds", 1, arguments, &out), "getBounds")) /* Condition for this action: Allocate a context-owned result object. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Allocate a context-owned result object. */
        goto done;                                                                              /* Allocate a context-owned result object. */
    } /* End conditional branch: Condition for this action: Allocate a context-owned result object. */
    if (out.type != FVM_VALUE_OBJECT || !out.object) /* Condition for this action: Validate before borrowing its pointer. */
    {                                                /* Begin conditional branch: Condition for this action: Validate before borrowing its pointer. */
        goto done;                                   /* Validate before borrowing its pointer. */
    } /* End conditional branch: Condition for this action: Validate before borrowing its pointer. */
    bounds = out.object;                                     /* Keep the returned object separate from later scalar results. */
    if (!checked(fvm_gc_pin(context, bounds), "pin bounds")) /* Condition for this action: [GC:] Retain the temporary result across property reads. */
    {                                                        /* Begin conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
        goto done;                                           /* [GC:] Retain the temporary result across property reads. */
    } /* End conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
    bounds_root = 1;                                                                         /* [GC:] Record ownership of the second explicit root. */
    if (!scalar(context, bounds, "xMin", &left) || !scalar(context, bounds, "xMax", &right)) /* Condition for this action: Copy the horizontal extents. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy the horizontal extents. */
        goto done;                                                                           /* Copy the horizontal extents. */
    } /* End conditional branch: Condition for this action: Copy the horizontal extents. */
    if (!scalar(context, bounds, "yMin", &top) || !scalar(context, bounds, "yMax", &bottom)) /* Condition for this action: Copy vertical analytic extrema. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy vertical analytic extrema. */
        goto done;                                                                           /* Copy vertical analytic extrema. */
    } /* End conditional branch: Condition for this action: Copy vertical analytic extrema. */
    if (left != 2 || right != 14 || top != 6 || bottom != 10) /* Condition for this action: Butt ends stop at x=2 and x=14; half-width expands only the normal axis. */
    {                                                         /* Begin conditional branch: Condition for this action: Butt ends stop at x=2 and x=14; half-width expands only the normal axis. */
        goto done;                                            /* Butt ends stop at x=2 and x=14; half-width expands only the normal axis. */
    } /* End conditional branch: Condition for this action: Butt ends stop at x=2 and x=14; half-width expands only the normal axis. */
    printf("axis curve bounds: %.0f..%.0f, %.0f..%.0f\n", left, right, top, bottom); /* Report the move-independent geometric result. */
    if (!checked(fvm_gc_unpin(context, bounds), "unpin old bounds"))                 /* Condition for this action: [GC:] Release the first result before replacing geometry. */
    {                                                                                /* Begin conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
        goto done;                                                                   /* [GC:] Release the first result before replacing geometry. */
    } /* End conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
    bounds_root = 0;                                                            /* [GC:] Cleanup no longer owns the first bounds root. */
    bounds = 0;                                                                 /* [GC:] Do not reuse the unrooted borrowed pointer. */
    if (!checked(fvm_object_call(context, clip, "clear", 0, 0, &out), "clear")) /* Condition for this action: Remove the preceding stroke before the independent diagonal query. */
    {                                                                           /* Begin conditional branch: Condition for this action: Remove the preceding stroke before the independent diagonal query. */
        goto done;                                                              /* Remove the preceding stroke before the independent diagonal query. */
    } /* End conditional branch: Condition for this action: Remove the preceding stroke before the independent diagonal query. */
    for (i = 0; i < 8; ++i)                 /* Default all optional parameters explicitly. */
    {                                       /* Begin loop body: Default all optional parameters explicitly. */
        fvm_value_undefined(arguments + i); /* Default all optional parameters explicitly. */
    } /* End loop body: Default all optional parameters explicitly. */
    fvm_value_number(arguments, 10);                                  /* Choose a ten-unit diagonal stroke. */
    fvm_value_number(arguments + 1, 0xff0000);                        /* Choose opaque red RGB. */
    fvm_value_number(arguments + 2, 100);                             /* Use one hundred percent opacity. */
    fvm_value_boolean(arguments + 3, 0);                              /* Keep unsupported pixel hinting disabled. */
    if (!checked(fvm_value_string(arguments + 4, "normal"), "scale")) /* Condition for this action: Borrow a static scale-mode string. */
    {                                                                 /* Begin conditional branch: Condition for this action: Borrow a static scale-mode string. */
        goto done;                                                    /* Borrow a static scale-mode string. */
    } /* End conditional branch: Condition for this action: Borrow a static scale-mode string. */
    if (!checked(fvm_value_string(arguments + 5, "none"), "cap")) /* Condition for this action: Select symmetric butt ends. */
    {                                                             /* Begin conditional branch: Condition for this action: Select symmetric butt ends. */
        goto done;                                                /* Select symmetric butt ends. */
    } /* End conditional branch: Condition for this action: Select symmetric butt ends. */
    if (!checked(fvm_value_string(arguments + 6, "bevel"), "join")) /* Condition for this action: Select bevel joins for this path. */
    {                                                               /* Begin conditional branch: Condition for this action: Select bevel joins for this path. */
        goto done;                                                  /* Select bevel joins for this path. */
    } /* End conditional branch: Condition for this action: Select bevel joins for this path. */
    if (!checked(fvm_object_call(context, clip, "lineStyle", 7, arguments, &out), "lineStyle")) /* Condition for this action: Copy the current style atomically. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Copy the current style atomically. */
        goto done;                                                                              /* Copy the current style atomically. */
    } /* End conditional branch: Condition for this action: Copy the current style atomically. */
    fvm_value_number(arguments, 2);                                                            /* Start the diagonal curve at x=2. */
    fvm_value_number(arguments + 1, 2);                                                        /* Start the diagonal at y=2. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "curve start")) /* Condition for this action: Begin a separate butt/bevel path. */
    {                                                                                          /* Begin conditional branch: Condition for this action: Begin a separate butt/bevel path. */
        goto done;                                                                             /* Begin a separate butt/bevel path. */
    } /* End conditional branch: Condition for this action: Begin a separate butt/bevel path. */
    fvm_value_number(arguments, 8);                                                         /* Place the quadratic control at x=8. */
    fvm_value_number(arguments + 1, 6.5);                                                   /* Keep the control at the exact diagonal midpoint. */
    fvm_value_number(arguments + 2, 14);                                                    /* End the quadratic at x=14. */
    fvm_value_number(arguments + 3, 11);                                                    /* Complete a 12-by-9 segment whose length is exactly 15. */
    if (!checked(fvm_object_call(context, clip, "curveTo", 4, arguments, &out), "curveTo")) /* Condition for this action: Copy the quadratic into the retained path. */
    {                                                                                       /* Begin conditional branch: Condition for this action: Copy the quadratic into the retained path. */
        goto done;                                                                          /* Copy the quadratic into the retained path. */
    } /* End conditional branch: Condition for this action: Copy the quadratic into the retained path. */
    fvm_value_number(arguments, 100);                                                            /* Choose a distant isolated move. */
    fvm_value_number(arguments + 1, 100);                                                        /* Keep the isolated point outside the actual stroke. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "isolated move")) /* Condition for this action: A move without a segment must not enlarge stroke bounds. */
    {                                                                                            /* Begin conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
        goto done;                                                                               /* A move without a segment must not enlarge stroke bounds. */
    } /* End conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
    fvm_value_undefined(arguments);                                                             /* Initialize the object-valued reference-space operand. */
    arguments[0].type = FVM_VALUE_OBJECT;                                                       /* Mark the following pointer as a managed object reference. */
    arguments[0].object = clip;                                                                 /* Borrow the already pinned clip as the bounds coordinate space. */
    if (!checked(fvm_object_call(context, clip, "getBounds", 1, arguments, &out), "getBounds")) /* Condition for this action: Allocate a context-owned result object. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Allocate a context-owned result object. */
        goto done;                                                                              /* Allocate a context-owned result object. */
    } /* End conditional branch: Condition for this action: Allocate a context-owned result object. */
    if (out.type != FVM_VALUE_OBJECT || !out.object) /* Condition for this action: Validate before borrowing its pointer. */
    {                                                /* Begin conditional branch: Condition for this action: Validate before borrowing its pointer. */
        goto done;                                   /* Validate before borrowing its pointer. */
    } /* End conditional branch: Condition for this action: Validate before borrowing its pointer. */
    bounds = out.object;                                     /* Keep the returned object separate from later scalar results. */
    if (!checked(fvm_gc_pin(context, bounds), "pin bounds")) /* Condition for this action: [GC:] Retain the temporary result across property reads. */
    {                                                        /* Begin conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
        goto done;                                           /* [GC:] Retain the temporary result across property reads. */
    } /* End conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
    bounds_root = 1;                                                                         /* [GC:] Record ownership of the second explicit root. */
    if (!scalar(context, bounds, "xMin", &left) || !scalar(context, bounds, "xMax", &right)) /* Condition for this action: Copy the horizontal extents. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy the horizontal extents. */
        goto done;                                                                           /* Copy the horizontal extents. */
    } /* End conditional branch: Condition for this action: Copy the horizontal extents. */
    if (!scalar(context, bounds, "yMin", &top) || !scalar(context, bounds, "yMax", &bottom)) /* Condition for this action: Copy vertical analytic extrema. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy vertical analytic extrema. */
        goto done;                                                                           /* Copy vertical analytic extrema. */
    } /* End conditional branch: Condition for this action: Copy vertical analytic extrema. */
    if (left != -1 || right != 17 || top != -2 || bottom != 15) /* Condition for this action: The five-unit normal offsets are exactly three horizontally and four vertically. */
    {                                                           /* Begin conditional branch: Condition for this action: The five-unit normal offsets are exactly three horizontally and four vertically. */
        goto done;                                              /* The five-unit normal offsets are exactly three horizontally and four vertically. */
    } /* End conditional branch: Condition for this action: The five-unit normal offsets are exactly three horizontally and four vertically. */
    printf("linear curve bounds: %.0f..%.0f, %.0f..%.0f\n", left, right, top, bottom); /* Report the move-independent geometric result. */
    if (!checked(fvm_gc_unpin(context, bounds), "unpin old bounds"))                   /* Condition for this action: [GC:] Release the first result before replacing geometry. */
    {                                                                                  /* Begin conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
        goto done;                                                                     /* [GC:] Release the first result before replacing geometry. */
    } /* End conditional branch: Condition for this action: [GC:] Release the first result before replacing geometry. */
    bounds_root = 0;                                                            /* [GC:] Cleanup no longer owns the first bounds root. */
    bounds = 0;                                                                 /* [GC:] Do not reuse the unrooted borrowed pointer. */
    if (!checked(fvm_object_call(context, clip, "clear", 0, 0, &out), "clear")) /* Condition for this action: Remove the preceding stroke before the independent diagonal query. */
    {                                                                           /* Begin conditional branch: Condition for this action: Remove the preceding stroke before the independent diagonal query. */
        goto done;                                                              /* Remove the preceding stroke before the independent diagonal query. */
    } /* End conditional branch: Condition for this action: Remove the preceding stroke before the independent diagonal query. */
    for (i = 0; i < 8; ++i)                 /* Default all optional parameters explicitly. */
    {                                       /* Begin loop body: Default all optional parameters explicitly. */
        fvm_value_undefined(arguments + i); /* Default all optional parameters explicitly. */
    } /* End loop body: Default all optional parameters explicitly. */
    fvm_value_number(arguments, 10);                                  /* Choose a ten-unit diagonal stroke. */
    fvm_value_number(arguments + 1, 0xff0000);                        /* Choose opaque red RGB. */
    fvm_value_number(arguments + 2, 100);                             /* Use one hundred percent opacity. */
    fvm_value_boolean(arguments + 3, 0);                              /* Keep unsupported pixel hinting disabled. */
    if (!checked(fvm_value_string(arguments + 4, "normal"), "scale")) /* Condition for this action: Borrow a static scale-mode string. */
    {                                                                 /* Begin conditional branch: Condition for this action: Borrow a static scale-mode string. */
        goto done;                                                    /* Borrow a static scale-mode string. */
    } /* End conditional branch: Condition for this action: Borrow a static scale-mode string. */
    if (!checked(fvm_value_string(arguments + 5, "none"), "cap")) /* Condition for this action: Select symmetric butt ends. */
    {                                                             /* Begin conditional branch: Condition for this action: Select symmetric butt ends. */
        goto done;                                                /* Select symmetric butt ends. */
    } /* End conditional branch: Condition for this action: Select symmetric butt ends. */
    if (!checked(fvm_value_string(arguments + 6, "bevel"), "join")) /* Condition for this action: Select bevel joins for this path. */
    {                                                               /* Begin conditional branch: Condition for this action: Select bevel joins for this path. */
        goto done;                                                  /* Select bevel joins for this path. */
    } /* End conditional branch: Condition for this action: Select bevel joins for this path. */
    if (!checked(fvm_object_call(context, clip, "lineStyle", 7, arguments, &out), "lineStyle")) /* Condition for this action: Copy the current style atomically. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Copy the current style atomically. */
        goto done;                                                                              /* Copy the current style atomically. */
    } /* End conditional branch: Condition for this action: Copy the current style atomically. */
    fvm_value_number(arguments, 2.00390625);                                                   /* Start on a fine dyadic grid beyond the old 1/16 lattice. */
    fvm_value_number(arguments + 1, 2.00390625);                                               /* Use the same exact fractional origin vertically. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "curve start")) /* Condition for this action: Begin a separate butt/bevel path. */
    {                                                                                          /* Begin conditional branch: Condition for this action: Begin a separate butt/bevel path. */
        goto done;                                                                             /* Begin a separate butt/bevel path. */
    } /* End conditional branch: Condition for this action: Begin a separate butt/bevel path. */
    fvm_value_number(arguments, 2.02734375);                                                /* Use the midpoint of a 12/256 horizontal span. */
    fvm_value_number(arguments + 1, 2.021484375);                                           /* Use the midpoint of a 9/256 vertical span. */
    fvm_value_number(arguments + 2, 2.05078125);                                            /* End at the exact fractional horizontal coordinate. */
    fvm_value_number(arguments + 3, 2.0390625);                                             /* Complete a segment of exact length 15/256. */
    if (!checked(fvm_object_call(context, clip, "curveTo", 4, arguments, &out), "curveTo")) /* Condition for this action: Copy the quadratic into the retained path. */
    {                                                                                       /* Begin conditional branch: Condition for this action: Copy the quadratic into the retained path. */
        goto done;                                                                          /* Copy the quadratic into the retained path. */
    } /* End conditional branch: Condition for this action: Copy the quadratic into the retained path. */
    fvm_value_number(arguments, 100);                                                            /* Choose a distant isolated move. */
    fvm_value_number(arguments + 1, 100);                                                        /* Keep the isolated point outside the actual stroke. */
    if (!checked(fvm_object_call(context, clip, "moveTo", 2, arguments, &out), "isolated move")) /* Condition for this action: A move without a segment must not enlarge stroke bounds. */
    {                                                                                            /* Begin conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
        goto done;                                                                               /* A move without a segment must not enlarge stroke bounds. */
    } /* End conditional branch: Condition for this action: A move without a segment must not enlarge stroke bounds. */
    fvm_value_undefined(arguments);                                                             /* Initialize the object-valued reference-space operand. */
    arguments[0].type = FVM_VALUE_OBJECT;                                                       /* Mark the following pointer as a managed object reference. */
    arguments[0].object = clip;                                                                 /* Borrow the already pinned clip as the bounds coordinate space. */
    if (!checked(fvm_object_call(context, clip, "getBounds", 1, arguments, &out), "getBounds")) /* Condition for this action: Allocate a context-owned result object. */
    {                                                                                           /* Begin conditional branch: Condition for this action: Allocate a context-owned result object. */
        goto done;                                                                              /* Allocate a context-owned result object. */
    } /* End conditional branch: Condition for this action: Allocate a context-owned result object. */
    if (out.type != FVM_VALUE_OBJECT || !out.object) /* Condition for this action: Validate before borrowing its pointer. */
    {                                                /* Begin conditional branch: Condition for this action: Validate before borrowing its pointer. */
        goto done;                                   /* Validate before borrowing its pointer. */
    } /* End conditional branch: Condition for this action: Validate before borrowing its pointer. */
    bounds = out.object;                                     /* Keep the returned object separate from later scalar results. */
    if (!checked(fvm_gc_pin(context, bounds), "pin bounds")) /* Condition for this action: [GC:] Retain the temporary result across property reads. */
    {                                                        /* Begin conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
        goto done;                                           /* [GC:] Retain the temporary result across property reads. */
    } /* End conditional branch: Condition for this action: [GC:] Retain the temporary result across property reads. */
    bounds_root = 1;                                                                         /* [GC:] Record ownership of the second explicit root. */
    if (!scalar(context, bounds, "xMin", &left) || !scalar(context, bounds, "xMax", &right)) /* Condition for this action: Copy the horizontal extents. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy the horizontal extents. */
        goto done;                                                                           /* Copy the horizontal extents. */
    } /* End conditional branch: Condition for this action: Copy the horizontal extents. */
    if (!scalar(context, bounds, "yMin", &top) || !scalar(context, bounds, "yMax", &bottom)) /* Condition for this action: Copy vertical analytic extrema. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Copy vertical analytic extrema. */
        goto done;                                                                           /* Copy vertical analytic extrema. */
    } /* End conditional branch: Condition for this action: Copy vertical analytic extrema. */
    if (left != -0.99609375 || right != 5.05078125 || top != -1.99609375 || bottom != 6.0390625) /* Condition for this action: The five-unit normal offsets are exactly three horizontally and four vertically. */
    {                                                                                            /* Begin conditional branch: Condition for this action: The five-unit normal offsets are exactly three horizontally and four vertically. */
        goto done;                                                                               /* The five-unit normal offsets are exactly three horizontally and four vertically. */
    } /* End conditional branch: Condition for this action: The five-unit normal offsets are exactly three horizontally and four vertically. */
    puts("adaptive curve bounds verified");  /* Report the move-independent geometric result. */
    result = 0;                              /* Every required geometry and available-renderer check succeeded. */
done:                                        /* Release only resources actually acquired by this host. */
    if (bounds_root)                         /* Condition for this action: [GC:] Drop the temporary result root first. */
    {                                        /* Begin conditional branch: Condition for this action: [GC:] Drop the temporary result root first. */
        (void)fvm_gc_unpin(context, bounds); /* [GC:] Drop the temporary result root first. */
    } /* End conditional branch: Condition for this action: [GC:] Drop the temporary result root first. */
    if (clip_root)                         /* Condition for this action: [GC:] Drop the graphics root before destroying the context. */
    {                                      /* Begin conditional branch: Condition for this action: [GC:] Drop the graphics root before destroying the context. */
        (void)fvm_gc_unpin(context, clip); /* [GC:] Drop the graphics root before destroying the context. */
    } /* End conditional branch: Condition for this action: [GC:] Drop the graphics root before destroying the context. */
    if (context)                      /* Condition for this action: Finalize all context-owned graphics and result objects. */
    {                                 /* Begin conditional branch: Condition for this action: Finalize all context-owned graphics and result objects. */
        fvm_context_destroy(context); /* Finalize all context-owned graphics and result objects. */
    } /* End conditional branch: Condition for this action: Finalize all context-owned graphics and result objects. */
    if (runtime)                      /* Condition for this action: Release the runtime after its context is gone. */
    {                                 /* Begin conditional branch: Condition for this action: Release the runtime after its context is gone. */
        fvm_runtime_destroy(runtime); /* Release the runtime after its context is gone. */
    } /* End conditional branch: Condition for this action: Release the runtime after its context is gone. */
    return result; /* Return zero only after complete success. */
} /* End the executable tutorial. */
