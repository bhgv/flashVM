/* FlashVM tutorial: retain a live two-pixel BitmapData as a MovieClip fill, render it and release
 * ownership. */
/* Declare runtime, context and value APIs used to create the interpreter and bitmap. */
#include "flashvm/standard.h"

/* Declare MovieClip construction and rendering APIs. */
#include "flashvm/movieclip.h"

/* [GC:] Declare explicit host roots used while objects are held by this example. */
#include "flashvm/gc.h"

/* Declare diagnostic output so every failed API call has a visible result code. */
#include <stdio.h>

/* Declare buffer initialization for the host-owned RGBA output image. */
#include <string.h>


/* Run the bitmap-fill tutorial; return a nonzero exit code if an API operation fails. */
int main(void)
/* Open the tutorial's ownership and cleanup scope. */
{ /* Begin scope: Open the tutorial's ownership and cleanup scope. */
    /* Store default configuration before creating the runtime. */
    FVM_Config config;
    /* Runtime remains null until creation succeeds, so cleanup is always safe. */
    FVM_Runtime *runtime = 0;
    /* Context owns all script objects, native bitmap buffers and standard classes. */
    FVM_Context *context = 0;
    /* [GC:] Hold the clip pointer while explicitly pinning it below. */
    FVM_Object *clip = 0;
    /* [GC:] Hold the source bitmap until the drawing style adopts its graph edge. */
    FVM_Object *bitmap = 0;
    /* Reuse initialized arguments for constructors, pixel writes and drawing commands. */
    FVM_Value args[4];
    /* Receive script method results, including undefined from drawing operations. */
    FVM_Value result;
    /* Hold 8-by-4 straight RGBA output pixels independently of the VM. */
    unsigned char pixels[8 * 4 * 4];
    /* Describe output dimensions and the 32-byte distance between rows. */
    FVM_RasterSurface surface = {0, 8, 4, 32};
    /* Save the current API status for diagnostics and the process exit code. */
    FVM_Result status;
    /* Select rectangle vertices in local MovieClip pixel coordinates. */
    const double vertices[10] = {0, 0, 8, 0, 8, 4, 0, 4, 0, 0};
    /* Track which pair of coordinates is supplied to the next drawing command. */
    unsigned int i;

    /* Initialize portable defaults before passing configuration to the runtime. */
    status = fvm_config_init(&config);
    /* Stop on configuration failure and release anything already created. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Create the runtime using those initialized defaults. */
    status = fvm_runtime_create(&config, &runtime);
    /* A failed runtime creation leaves runtime null for cleanup. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Create an interpreter context owned by the runtime. */
    status = fvm_context_create(runtime, &context);
    /* Handle context allocation failure without continuing with an invalid pointer. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Install MovieClip, BitmapData and their retained native methods in this context. */
    status = fvm_standard_install(context);
    /* Leave on standard-class installation failure. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Create a one-frame root MovieClip named texture. */
    status = fvm_movieclip_create(context, "texture", 1, &clip);
    /* Check clip construction before retaining or drawing through it. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* [GC:] Pin the clip while the host retains it outside script globals. */
    status = fvm_gc_pin(context, clip);
    /* [GC:] Do not proceed if the explicit host root cannot be installed. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Request a two-pixel-wide bitmap. */
    fvm_value_number(args, 2);
    /* Request a one-pixel-high bitmap. */
    fvm_value_number(args + 1, 1);
    /* Retain alpha so this example can be extended to transparent textures. */
    fvm_value_boolean(args + 2, 1);
    /* Fill both pixels with opaque red before changing the second pixel. */
    fvm_value_number(args + 3, 4294901760.0);
    /* Construct BitmapData with width, height, transparency and ARGB fill color. */
    status = fvm_construct(context, "flash.display.BitmapData", 4, args, &bitmap);
    /* Check construction before referencing the bitmap. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* [GC:] Pin the bitmap during setup; the drawing style will later retain it itself. */
    status = fvm_gc_pin(context, bitmap);
    /* [GC:] Stop if its temporary host root cannot be registered. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Select the second source pixel at horizontal coordinate one. */
    fvm_value_number(args, 1);
    /* Select the only source row at vertical coordinate zero. */
    fvm_value_number(args + 1, 0);
    /* Choose opaque green for the second source pixel. */
    fvm_value_number(args + 2, 4278255360.0);
    /* Write (x=1, y=0, ARGB=opaque green) into the live source bitmap. */
    status = fvm_object_call(context, bitmap, "setPixel32", 3, args, &result);
    /* Check the pixel write before installing its drawing style. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Initialize a complete value before assigning the bitmap object reference. */
    fvm_value_undefined(args);
    /* Identify the first fill argument as a script object. */
    args[0].type = FVM_VALUE_OBJECT;
    /* [GC:] Supply the bitmap whose managed edge will be retained by the fill descriptor. */
    args[0].object = bitmap;
    /* Use the default identity matrix for pixel-aligned sampling. */
    fvm_value_undefined(args + 1);
    /* Tile the two source pixels across the rectangle. */
    fvm_value_boolean(args + 2, 1);
    /* Use nearest-neighbor sampling so the alternating colors stay sharp. */
    fvm_value_boolean(args + 3, 0);
    /* Set (bitmap, default matrix, repeat=true, smoothing=false) on the clip. */
    status = fvm_object_call(context, clip, "beginBitmapFill", 4, args, &result);
    /* Leave the source pinned if installing the style failed; context cleanup still owns it. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* [GC:] The clip now retains the bitmap through its drawing descriptor, replacing this host
     * root. */
    status = fvm_gc_unpin(context, bitmap);
    /* [GC:] Check ownership transfer before proceeding with drawing. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Walk five vertices: move to the first, then draw four rectangle edges. */
    for (i = 0; i < 10; i += 2)
    /* Open one drawing-command iteration. */
    { /* Begin scope: Open one drawing-command iteration. */
        /* Supply the vertex's horizontal coordinate. */
        fvm_value_number(args, vertices[i]);
        /* Supply the vertex's vertical coordinate. */
        fvm_value_number(args + 1, vertices[i + 1]);
        /* Call moveTo for the first vertex or lineTo for a subsequent edge endpoint. */
        status = fvm_object_call(context, clip, i ? "lineTo" : "moveTo", 2, args, &result);
        /* Handle a drawing allocation or coordinate error immediately. */
        if (status != FVM_OK)
        { /* Begin conditional branch: if (status != FVM_OK). */
            /* Release initialized context/runtime resources and return the recorded failure. */
            goto cleanup;
        } /* End conditional branch: if (status != FVM_OK). */
        /* Close this vertex iteration. */
    } /* End scope: Open one drawing-command iteration. */
    /* Finish the current style; the completed path retains its live bitmap independently. */
    status = fvm_object_call(context, clip, "endFill", 0, 0, &result);
    /* Check completion before rendering. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Release initialized context/runtime resources and return the recorded failure. */
        goto cleanup;
    } /* End conditional branch: if (status != FVM_OK). */
    /* Clear the host-owned image to transparent black. */
    memset(pixels, 0, sizeof(pixels));
    /* Point the surface descriptor to its actual host-owned pixel array. */
    surface.pixels = pixels;
    /* Render the clip into an 8-by-4 RGBA image with a 32-byte row stride. */
    status = fvm_movieclip_render(context, clip, &surface);
    /* A graphics-disabled profile still stores the complete bitmap drawing graph. */
    if (status == FVM_UNSUPPORTED)
    /* Open the headless-result branch. */
    { /* Begin scope: Open the headless-result branch. */
        /* Explain why no rendered pixels are available in this build profile. */
        puts("Bitmap fill stored; this profile has no graphics backend.");
        /* Treat the expected unsupported renderer as a successful tutorial result. */
        status = FVM_OK;
        /* Close the headless-result branch. */
    } /* End scope: Open the headless-result branch. */
    /* Report the first two colors after successful graphical rendering. */
    else if (status == FVM_OK)
    /* Open the successful graphical-output branch. */
    { /* Begin scope: Open the successful graphical-output branch. */
        /* Print the first pixel's three RGB bytes at offsets zero through two. */
        printf("RGB: %u,%u,%u / ", (unsigned)pixels[0], (unsigned)pixels[1], (unsigned)pixels[2]);
        /* Print the adjacent pixel's RGB bytes at offsets four through six. */
        printf("second: %u,%u,%u\n", (unsigned)pixels[4], (unsigned)pixels[5], (unsigned)pixels[6]);
        /* Close the successful graphical-output branch. */
    } /* End scope: Open the successful graphical-output branch. */


/* Funnel success and all error paths through the same ownership cleanup. */
cleanup:
    /* Report an API failure with its numeric result code. */
    if (status != FVM_OK)
    { /* Begin conditional branch: if (status != FVM_OK). */
        /* Send errors to stderr rather than hiding them behind a disabled assertion. */
        fprintf(stderr, "Bitmap fill failed: %d\n", (int)status);
    } /* End conditional branch: if (status != FVM_OK). */
    /* [GC:] Destroy the context, releasing its clips, bitmap storage and remaining host roots. */
    if (context)
    { /* Begin conditional branch: if (context). */
        /* [GC:] Release all context-owned objects, their native pixel buffers and host roots. */
        fvm_context_destroy(context);
    } /* End conditional branch: if (context). */
    /* Release runtime services after their last context is gone. */
    if (runtime)
    { /* Begin conditional branch: if (runtime). */
        /* Release the runtime after the interpreter context has been destroyed. */
        fvm_runtime_destroy(runtime);
    } /* End conditional branch: if (runtime). */
    /* Map the final API status to the process exit code. */
    return status == FVM_OK ? 0 : 1;
    /* Close the tutorial's main function. */
} /* End scope: Open the tutorial's ownership and cleanup scope. */
