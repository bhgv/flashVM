/* C89 standalone image replacement, native RGBA rendering and unloading; see README.md. */
#include "flashvm/standard.h" /* Runtime, native classes and scalar properties. */
#include "flashvm/movieclip.h" /* Atomic image/resource replacement and rendering contracts. */
#include "flashvm/gc.h" /* Explicit retention across host calls and collection. */
#include <stdio.h> /* Report ordinary failures and a deterministic result. */
#include <string.h> /* Initialize caller-owned RGBA storage. */
#include "picture.inc" /* Immutable PNG input; never released or modified by the VM. */
/* Run without SDL, a network provider, an AS compiler or a temporary image file. */
int main(void) /* Build target: flashvm_examples_embedding_image_codecs_loading_main. */
{ /* Initialize partial ownership before the first fallible call. */
    FVM_Config config; /* Runtime copies this caller-owned descriptor. */
    FVM_Runtime *runtime = 0; /* Outlive the managed context. */
    FVM_Context *context = 0; /* Own clip state, bitmap pixels and its host pin. */
    FVM_Object *clip = 0; /* Borrow this identity until context destruction. */
    FVM_MovieClipContentInfo info; /* Copied scalar metadata remains valid independently of the clip. */
    FVM_Value value; /* Carry copied scalars and borrowed source strings between host calls. */
    unsigned char pixels[16]; /* Four caller-owned straight RGBA pixels. */
    FVM_RasterSurface surface = {0, 2, 2, 8}; /* Borrow output storage only during rendering. */
    FVM_Result status = fvm_config_init(&config); /* Initialize the public configuration ABI. */
    surface.pixels = pixels; /* Bind automatic storage after C89 declarations finish. */
    if (status == FVM_OK) /* Initialize the required descriptor ABI before its first query. */
        status = fvm_movieclip_content_info_init(&info); /* No context, allocation or borrowed pointers. */
    if (status == FVM_OK) /* Keep setup errors observable through common cleanup. */
        status = fvm_runtime_create(&config, &runtime); /* Create the enclosing owner. */
    if (status == FVM_OK) /* Context requires a live runtime. */
        status = fvm_context_create(runtime, &context); /* Allocate managed storage on this thread. */
    if (status == FVM_OK) /* Install trusted native MovieClip and BitmapData classes. */
        status = fvm_standard_install(context); /* Replacement bypasses replaceable constructors. */
    if (status == FVM_OK) /* A native clip has no source SWF profile restriction. */
        status = fvm_movieclip_create(context, "picture", 1, &clip); /* Borrow the new instance. */
    if (status == FVM_OK) /* [GC:] Retain the graph across later calls and explicit collection. */
        status = fvm_gc_pin(context, clip); /* Context destruction also releases this pin. */
    fvm_value_number(&value, 42); /* Demonstrate data which belongs to the instance. */
    if (status == FVM_OK) /* Set own state before changing the content graph. */
        status = fvm_object_set(context, clip, "kept", &value); /* Copy the scalar property. */
    if (status == FVM_OK) /* Owner safe point outside native/script callbacks. */
        status = fvm_movieclip_replace_image(context, clip, picture_png, sizeof(picture_png), "embedded.png", 12); /* Borrow counted bytes. */
    if (status == FVM_OK) /* The generic entrypoint selects the same image by its signature. */
        status = fvm_movieclip_replace_resource(context, clip, picture_png, sizeof(picture_png), "resource", 8); /* Preserve identity again. */
    if (status == FVM_OK) /* Read the current painted extent through the standard object API. */
        status = fvm_object_get(context, clip, "_width", &value); /* Named geometry is not a cached scalar. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != 2)) /* PNG width is two pixels. */
        status = FVM_ERROR; /* Keep an unexpected getter result observable. */
    fvm_value_number(&value, 4); /* Request twice the original width. */
    if (status == FVM_OK) /* The same named setter is available to an IDE or C application. */
        status = fvm_object_set(context, clip, "_width", &value); /* Convert once and update native scales. */
    if (status == FVM_OK) /* Confirm the next read reflects the resized geometry. */
        status = fvm_object_get(context, clip, "_width", &value); /* No console command or AS compiler. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != 4)) /* Expect the requested width. */
        status = FVM_ERROR; /* Preserve failed verification through cleanup. */
    fvm_value_number(&value, 2); /* Restore original dimensions for the deterministic pixel comparison. */
    if (status == FVM_OK) /* Width changes keep the same managed image and clip identity. */
        status = fvm_object_set(context, clip, "_width", &value); /* Return the horizontal scale to 100 percent. */
    if (status == FVM_OK) /* Source comes from the last successful content publication. */
        status = fvm_object_get(context, clip, "_url", &value); /* Borrow managed text only until the next collecting call. */
    if (status == FVM_OK && (value.type != FVM_VALUE_STRING || fvm_value_string_size(&value) != 8 || memcmp(value.string, "resource", 8))) /* Compare counted source. */
        status = FVM_ERROR; /* No strlen on binary movie bytes or borrowed unvalidated values. */
    if (status == FVM_OK) /* Read only committed source statistics, independent of live transport. */
        status = fvm_movieclip_get_content_info(context, clip, &info); /* Copy all fields atomically on success. */
    if (status == FVM_OK && /* Inspect a fully copied successful snapshot. */
        (info.format != FVM_MOVIECLIP_CONTENT_PNG || /* Signature identifies the imported PNG. */
         info.encoded_bytes != sizeof(picture_png) || /* Count the complete input, including its header. */
         info.decoded_bytes != sizeof(pixels) || /* Original two-by-two straight RGBA payload. */
         info.pixel_width != 2 || info.pixel_height != 2 || /* Geometry changes did not resize original pixels. */
         info.swf_version != 0)) /* An image has no SWF header version. */
        status = FVM_ERROR; /* A misleading snapshot must fail the tutorial. */
    if (status == FVM_OK) /* [GC:] The host pin must keep the bitmap drawing graph alive. */
        status = fvm_gc_collect(context, 0); /* The bitmap's owned pixels remain reachable. */
    memset(pixels, 0, sizeof(pixels)); /* Render onto a transparent caller-owned surface. */
    if (status == FVM_OK) /* Pixel operations run synchronously on the owner thread. */
        status = fvm_movieclip_render(context, clip, &surface); /* No input buffer is retained by this call. */
    if (status == FVM_OK && (pixels[0] != 255 || pixels[3] != 255)) /* Check the known opaque red corner. */
        status = FVM_ERROR; /* Use ordinary error handling rather than tutorial assertions. */
    if (status == FVM_OK) /* Remove the image while retaining the instance. */
        status = fvm_movieclip_unload(context, clip); /* No script unload event is synthesized. */
    if (status == FVM_OK) /* An explicit unload has known zero payload, unlike an unmarked native clip. */
        status = fvm_movieclip_get_content_info(context, clip, &info); /* Reuse the caller-owned descriptor. */
    if (status == FVM_OK && /* The successful unload published a complete empty record. */
        (info.format != FVM_MOVIECLIP_CONTENT_EMPTY || /* Distinguish explicit unload from unknown metadata. */
         info.encoded_bytes != 0 || info.decoded_bytes != 0)) /* No input or decoded payload remains. */
        status = FVM_ERROR; /* Preserve the error through normal cleanup. */
    if (status == FVM_OK) /* Verify own data survived both replacement and clearing. */
        status = fvm_object_get(context, clip, "kept", &value); /* Scalar output needs no GC root. */
    if (status == FVM_OK && (value.type != FVM_VALUE_NUMBER || value.number != 42)) /* Validate preserved instance data. */
        status = FVM_ERROR; /* Preserve failure through cleanup and the exit code. */
    if (status == FVM_OK) /* Report only completed work. */
        puts("image pixels verified; instance data retained"); /* Expected deterministic output. */
    else /* Do not conceal unsupported codec builds or decode failures. */
        fprintf(stderr, "image tutorial failed: %d\n", (int)status); /* Managed storage is not used for diagnostics. */
    if (context) /* [GC:] Release graph and pin before destroying the runtime. */
        fvm_context_destroy(context); /* No clip/pixel pointer is accessed afterwards. */
    if (runtime) /* Release the enclosing owner last. */
        fvm_runtime_destroy(runtime); /* Static input and stack RGBA storage remain caller-owned. */
    return status == FVM_OK ? 0 : 1; /* Let shell/CTest observe actual success or failure. */
} /* End this independent owner-thread example. */
