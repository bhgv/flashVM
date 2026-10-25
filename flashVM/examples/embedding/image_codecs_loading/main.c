/* C89 standalone image replacement, native RGBA rendering and unloading; see README.md. */
#include "flashvm/standard.h"  /* Runtime, native classes and scalar properties. */
#include "flashvm/movieclip.h" /* Atomic image/resource replacement and rendering contracts. */
#include "flashvm/gc.h"        /* Explicit retention across host calls and collection. */
#include <stdio.h>             /* Report ordinary failures and a deterministic result. */
#include <string.h>            /* Initialize caller-owned RGBA storage. */
#include "picture.inc"         /* Immutable PNG input; never released or modified by the VM. */


/* Run without SDL, a network provider, an AS compiler or a temporary image file. */
int main(void)                                           /* Build target: flashvm_examples_embedding_image_codecs_loading_main. */
{                                                        /* Initialize partial ownership before the first fallible call. */
    FVM_Config config;                                   /* Runtime copies this caller-owned descriptor. */
    FVM_Runtime *runtime = 0;                            /* Outlive the managed context. */
    FVM_Context *context = 0;                            /* Own clip state, bitmap pixels and its host pin. */
    FVM_Object *clip = 0;                                /* Borrow this identity until context destruction. */
    FVM_MovieClipContentInfo info;                       /* Copied scalar metadata remains valid independently of the clip. */
    FVM_Value value;                                     /* Carry copied scalars and borrowed source strings between host calls. */
    unsigned char pixels[16];                            /* Four caller-owned straight RGBA pixels. */
    FVM_RasterSurface surface = {0, 2, 2, 8};            /* Borrow output storage only during rendering. */
    FVM_Result status = fvm_config_init(&config);        /* Initialize the public configuration ABI. */
    surface.pixels = pixels;                             /* Bind automatic storage after C89 declarations finish. */
    if (status == FVM_OK)                                /* Initialize the required descriptor ABI before its first query. */
    {                                                    /* Begin conditional branch: Initialize the required descriptor ABI before its first query. */
        status = fvm_movieclip_content_info_init(&info); /* No context, allocation or borrowed pointers. */
    } /* End conditional branch: Initialize the required descriptor ABI before its first query. */
    if (status == FVM_OK)                               /* Keep setup errors observable through common cleanup. */
    {                                                   /* Begin conditional branch: Keep setup errors observable through common cleanup. */
        status = fvm_runtime_create(&config, &runtime); /* Create the enclosing owner. */
    } /* End conditional branch: Keep setup errors observable through common cleanup. */
    if (status == FVM_OK)                               /* Context requires a live runtime. */
    {                                                   /* Begin conditional branch: Context requires a live runtime. */
        status = fvm_context_create(runtime, &context); /* Allocate managed storage on this thread. */
    } /* End conditional branch: Context requires a live runtime. */
    if (status == FVM_OK)                       /* Install trusted native MovieClip and BitmapData classes. */
    {                                           /* Begin conditional branch: Install trusted native MovieClip and BitmapData classes. */
        status = fvm_standard_install(context); /* Replacement bypasses replaceable constructors. */
    } /* End conditional branch: Install trusted native MovieClip and BitmapData classes. */
    if (status == FVM_OK)                                            /* A native clip has no source SWF profile restriction. */
    {                                                                /* Begin conditional branch: A native clip has no source SWF profile restriction. */
        status = fvm_movieclip_create(context, "picture", 1, &clip); /* Borrow the new instance. */
    } /* End conditional branch: A native clip has no source SWF profile restriction. */
    if (status == FVM_OK)                   /* [GC:] Retain the graph across later calls and explicit collection. */
    {                                       /* Begin conditional branch: [GC:] Retain the graph across later calls and explicit collection. */
        status = fvm_gc_pin(context, clip); /* Context destruction also releases this pin. */
    } /* End conditional branch: [GC:] Retain the graph across later calls and explicit collection. */
    fvm_value_number(&value, 42);                               /* Demonstrate data which belongs to the instance. */
    if (status == FVM_OK)                                       /* Set own state before changing the content graph. */
    {                                                           /* Begin conditional branch: Set own state before changing the content graph. */
        status = fvm_object_set(context, clip, "kept", &value); /* Copy the scalar property. */
    } /* End conditional branch: Set own state before changing the content graph. */
    if (status == FVM_OK)                                                                                          /* Owner safe point outside native/script callbacks. */
    {                                                                                                              /* Begin conditional branch: Owner safe point outside native/script callbacks. */
        status = fvm_movieclip_replace_image(context, clip, picture_png, sizeof(picture_png), "embedded.png", 12); /* Borrow counted bytes. */
    } /* End conditional branch: Owner safe point outside native/script callbacks. */
    if (status == FVM_OK)                                                                                        /* The generic entrypoint selects the same image by its signature. */
    {                                                                                                            /* Begin conditional branch: The generic entrypoint selects the same image by its signature. */
        status = fvm_movieclip_replace_resource(context, clip, picture_png, sizeof(picture_png), "resource", 8); /* Preserve identity again. */
    } /* End conditional branch: The generic entrypoint selects the same image by its signature. */
    if (status == FVM_OK)                                         /* Read the current painted extent through the standard object API. */
    {                                                             /* Begin conditional branch: Read the current painted extent through the standard object API. */
        status = fvm_object_get(context, clip, "_width", &value); /* Named geometry is not a cached scalar. */
    } /* End conditional branch: Read the current painted extent through the standard object API. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != 2)) /* PNG width is two pixels. */
    {                                                                              /* Begin conditional branch: PNG width is two pixels. */
        status = FVM_ERROR;                                                        /* Keep an unexpected getter result observable. */
    } /* End conditional branch: PNG width is two pixels. */
    fvm_value_number(&value, 4);                                  /* Request twice the original width. */
    if (status == FVM_OK)                                         /* The same named setter is available to an IDE or C application. */
    {                                                             /* Begin conditional branch: The same named setter is available to an IDE or C application. */
        status = fvm_object_set(context, clip, "_width", &value); /* Convert once and update native scales. */
    } /* End conditional branch: The same named setter is available to an IDE or C application. */
    if (status == FVM_OK)                                         /* Confirm the next read reflects the resized geometry. */
    {                                                             /* Begin conditional branch: Confirm the next read reflects the resized geometry. */
        status = fvm_object_get(context, clip, "_width", &value); /* No console command or AS compiler. */
    } /* End conditional branch: Confirm the next read reflects the resized geometry. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != 4)) /* Expect the requested width. */
    {                                                                              /* Begin conditional branch: Expect the requested width. */
        status = FVM_ERROR;                                                        /* Preserve failed verification through cleanup. */
    } /* End conditional branch: Expect the requested width. */
    fvm_value_number(&value, 2);                                  /* Restore original dimensions for the deterministic pixel comparison. */
    if (status == FVM_OK)                                         /* Width changes keep the same managed image and clip identity. */
    {                                                             /* Begin conditional branch: Width changes keep the same managed image and clip identity. */
        status = fvm_object_set(context, clip, "_width", &value); /* Return the horizontal scale to 100 percent. */
    } /* End conditional branch: Width changes keep the same managed image and clip identity. */
    if (status == FVM_OK)                                       /* Source comes from the last successful content publication. */
    {                                                           /* Begin conditional branch: Source comes from the last successful content publication. */
        status = fvm_object_get(context, clip, "_url", &value); /* Borrow managed text only until the next collecting call. */
    } /* End conditional branch: Source comes from the last successful content publication. */
    if (status == FVM_OK && (value.type != FVM_VALUE_STRING || fvm_value_string_size(&value) != 8 || memcmp(value.string, "resource", 8))) /* Compare counted source. */
    {                                                                                                                                      /* Begin conditional branch: Compare counted source. */
        status = FVM_ERROR;                                                                                                                /* No strlen on binary movie bytes or borrowed unvalidated values. */
    } /* End conditional branch: Compare counted source. */
    if (status == FVM_OK)                                              /* Read only committed source statistics, independent of live transport. */
    {                                                                  /* Begin conditional branch: Read only committed source statistics, independent of live transport. */
        status = fvm_movieclip_get_content_info(context, clip, &info); /* Copy all fields atomically on success. */
    } /* End conditional branch: Read only committed source statistics, independent of live transport. */
    if (status == FVM_OK &&                                 /* Inspect a fully copied successful snapshot. */
        (info.format != FVM_MOVIECLIP_CONTENT_PNG ||        /* Signature identifies the imported PNG. */
         info.encoded_bytes != sizeof(picture_png) ||       /* Count the complete input, including its header. */
         info.decoded_bytes != sizeof(pixels) ||            /* Original two-by-two straight RGBA payload. */
         info.pixel_width != 2 || info.pixel_height != 2 || /* Geometry changes did not resize original pixels. */
         info.swf_version != 0))                            /* An image has no SWF header version. */
    {                                                       /* Begin scope: An image has no SWF header version. */
        status = FVM_ERROR;                                 /* A misleading snapshot must fail the tutorial. */
    } /* End scope: An image has no SWF header version. */
    if (status == FVM_OK)                    /* [GC:] The host pin must keep the bitmap drawing graph alive. */
    {                                        /* Begin conditional branch: [GC:] The host pin must keep the bitmap drawing graph alive. */
        status = fvm_gc_collect(context, 0); /* The bitmap's owned pixels remain reachable. */
    } /* End conditional branch: [GC:] The host pin must keep the bitmap drawing graph alive. */
    memset(pixels, 0, sizeof(pixels));                          /* Render onto a transparent caller-owned surface. */
    if (status == FVM_OK)                                       /* Pixel operations run synchronously on the owner thread. */
    {                                                           /* Begin conditional branch: Pixel operations run synchronously on the owner thread. */
        status = fvm_movieclip_render(context, clip, &surface); /* No input buffer is retained by this call. */
    } /* End conditional branch: Pixel operations run synchronously on the owner thread. */
    if (status == FVM_OK && (pixels[0] != 255 || pixels[3] != 255)) /* Check the known opaque red corner. */
    {                                                               /* Begin conditional branch: Check the known opaque red corner. */
        status = FVM_ERROR;                                         /* Use ordinary error handling rather than tutorial assertions. */
    } /* End conditional branch: Check the known opaque red corner. */
    if (status == FVM_OK)                             /* Remove the image while retaining the instance. */
    {                                                 /* Begin conditional branch: Remove the image while retaining the instance. */
        status = fvm_movieclip_unload(context, clip); /* No script unload event is synthesized. */
    } /* End conditional branch: Remove the image while retaining the instance. */
    if (status == FVM_OK)                                              /* An explicit unload has known zero payload, unlike an unmarked native clip. */
    {                                                                  /* Begin conditional branch: An explicit unload has known zero payload, unlike an unmarked native clip. */
        status = fvm_movieclip_get_content_info(context, clip, &info); /* Reuse the caller-owned descriptor. */
    } /* End conditional branch: An explicit unload has known zero payload, unlike an unmarked native clip. */
    if (status == FVM_OK &&                                   /* The successful unload published a complete empty record. */
        (info.format != FVM_MOVIECLIP_CONTENT_EMPTY ||        /* Distinguish explicit unload from unknown metadata. */
         info.encoded_bytes != 0 || info.decoded_bytes != 0)) /* No input or decoded payload remains. */
    {                                                         /* Begin scope: No input or decoded payload remains. */
        status = FVM_ERROR;                                   /* Preserve the error through normal cleanup. */
    } /* End scope: No input or decoded payload remains. */
    if (status == FVM_OK)                                       /* Verify own data survived both replacement and clearing. */
    {                                                           /* Begin conditional branch: Verify own data survived both replacement and clearing. */
        status = fvm_object_get(context, clip, "kept", &value); /* Scalar output needs no GC root. */
    } /* End conditional branch: Verify own data survived both replacement and clearing. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != 42)) /* Validate preserved instance data. */
    {                                                                               /* Begin conditional branch: Validate preserved instance data. */
        status = FVM_ERROR;                                                         /* Preserve failure through cleanup and the exit code. */
    } /* End conditional branch: Validate preserved instance data. */
    if (status == FVM_OK)                                      /* Report only completed work. */
    {                                                          /* Begin conditional branch: Report only completed work. */
        puts("image pixels verified; instance data retained"); /* Expected deterministic output. */
    } /* End conditional branch: Report only completed work. */
    else                                                             /* Do not conceal unsupported codec builds or decode failures. */
    {                                                                /* Begin conditional branch: Do not conceal unsupported codec builds or decode failures. */
        fprintf(stderr, "image tutorial failed: %d\n", (int)status); /* Managed storage is not used for diagnostics. */
    } /* End conditional branch: Do not conceal unsupported codec builds or decode failures. */
    if (context)                      /* [GC:] Release graph and pin before destroying the runtime. */
    {                                 /* Begin conditional branch: [GC:] Release graph and pin before destroying the runtime. */
        fvm_context_destroy(context); /* No clip/pixel pointer is accessed afterwards. */
    } /* End conditional branch: [GC:] Release graph and pin before destroying the runtime. */
    if (runtime)                      /* Release the enclosing owner last. */
    {                                 /* Begin conditional branch: Release the enclosing owner last. */
        fvm_runtime_destroy(runtime); /* Static input and stack RGBA storage remain caller-owned. */
    } /* End conditional branch: Release the enclosing owner last. */
    return status == FVM_OK ? 0 : 1; /* Let shell/CTest observe actual success or failure. */
} /* End this independent owner-thread example. */
