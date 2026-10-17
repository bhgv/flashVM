/* Compare vector edge coverage before and after the legacy ToggleQuality action. */
#include "flashvm/standard.h" /* Declare runtime setup, typed values and native calls. */
#include "flashvm/movieclip.h" /* Declare host-authored MovieClip geometry and rendering. */
#include "flashvm/stage.h" /* Declare context-local quality selection and inspection. */
#include "flashvm/gc.h" /* [GC:] Retain the host clip through arbitrary execution safe points. */
#include "flashvm/avm1.h" /* Execute the same ToggleQuality operation used by a SWF. */
#include <stdio.h> /* Report ordinary errors and edge-pixel counts. */
#include <string.h> /* Clear the host-owned output surface before each rendering pass. */
#define FVM_EXAMPLE_QUALITY_WIDTH 8U /* Keep the diagnostic surface eight pixels wide. */
#define FVM_EXAMPLE_QUALITY_HEIGHT 8U /* Keep the diagnostic surface eight pixels high. */
#define FVM_EXAMPLE_QUALITY_RGBA 4U /* Store straight RGBA bytes for each pixel. */

/* Print a failed API result and let main follow its ordinary cleanup path. */
static int checked(FVM_Result result, const char *operation) /* Borrow the operation label. */
{ /* Begin checked result reporting. */
    if (result == FVM_OK) /* Successful operations need no error message. */
        return 1; /* Allow the caller to continue setup or rendering. */
    fprintf(stderr, "%s: result %d\n", operation, (int)result); /* Keep the failing result visible. */
    return 0; /* Ask the caller to release its owned handles. */
} /* End the ordinary error helper. */

/* Count partially covered edge pixels without assuming backend-exact RGB rounding. */
static unsigned int edge_pixels(const unsigned char *pixels) /* Borrow one complete RGBA image. */
{ /* Begin the alpha-only coverage query. */
    unsigned int i, count = 0; /* Enumerate pixels and count partial coverage. */
    for (i = 0; i < FVM_EXAMPLE_QUALITY_WIDTH * FVM_EXAMPLE_QUALITY_HEIGHT; ++i) /* Visit every pixel. */
    { /* Inspect the pixel's alpha byte. */
        unsigned int alpha = pixels[i * FVM_EXAMPLE_QUALITY_RGBA + 3U]; /* Read straight opacity. */
        if (alpha != 0 && alpha != 255) /* Opaque paint gains partial alpha only at antialiased edges. */
            ++count; /* Count this partially covered pixel once. */
    } /* Finish the complete image scan. */
    return count; /* Return the diagnostic count, without accessing VM state. */
} /* End the coverage helper. */

/* Retain one triangle and compare HIGH with action-selected LOW. */
int main(void) /* No command line input or external SWF file is required. */
{ /* Begin explicit runtime and host-image ownership. */
    const double triangle[] = {0.2, 0.3, 7.6, 1.4, 2.3, 7.7, 0.2, 0.3}; /* Use fractional diagonal edges. */
    const unsigned char toggle[] = {0x08, 0x00}; /* Independent wire fixture: ToggleQuality followed by End. */
    unsigned char pixels[FVM_EXAMPLE_QUALITY_WIDTH * FVM_EXAMPLE_QUALITY_HEIGHT * FVM_EXAMPLE_QUALITY_RGBA]; /* Own the output bytes. */
    FVM_RasterSurface surface; /* Borrow pixels for the synchronous render call. */
    FVM_Config config; /* Initialize configuration before creating a runtime. */
    FVM_Runtime *runtime = 0; /* Track runtime ownership even during partial setup. */
    FVM_Context *context = 0; /* Own the managed graph and context-wide quality policy. */
    FVM_Object *clip = 0; /* Borrow the managed clip while its context remains alive. */
    FVM_Value arguments[2], output; /* Hold drawing arguments and ordinary call output. */
    FVM_StageQuality quality; /* Receive the actual quality selected by the action. */
    FVM_Result result; /* Separate a disabled raster backend from an unexpected error. */
    unsigned int i, high_edges, low_edges; /* Track geometry coordinates and initial edge coverage. */
    int clip_pinned = 0; /* [GC:] Track the explicit host root for partial setup cleanup. */
    int exit_code = 1; /* Report success only after the demonstrated operations complete. */
    surface.pixels = pixels; /* Point the view at caller-owned memory. */
    surface.width = FVM_EXAMPLE_QUALITY_WIDTH; /* Publish the buffer's pixel width. */
    surface.height = FVM_EXAMPLE_QUALITY_HEIGHT; /* Publish the buffer's pixel height. */
    surface.stride = FVM_EXAMPLE_QUALITY_WIDTH * FVM_EXAMPLE_QUALITY_RGBA; /* Use tightly packed rows. */
    if (!checked(fvm_config_init(&config), "config_init")) /* Initialize the public configuration ABI. */
        goto cleanup; /* No VM handles exist yet. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Acquire the runtime. */
        goto cleanup; /* Release only handles that were actually created. */
    if (!checked(fvm_context_create(runtime, &context), "context_create")) /* Acquire one VM context. */
        goto cleanup; /* Keep the runtime available until context cleanup finishes. */
    if (!checked(fvm_standard_install(context), "standard_install")) /* Install MovieClip's ordinary object methods. */
        goto cleanup; /* Stop before using an incomplete standard environment. */
    if (!checked(fvm_movieclip_create(context, "triangle", 1, &clip), "movieclip_create")) /* Create one host-authored timeline. */
        goto cleanup; /* Its managed lifetime belongs to the context. */
    if (!checked(fvm_gc_pin(context, clip), "gc_pin")) /* [GC:] Keep the clip reachable while the host holds its pointer. */
        goto cleanup; /* [GC:] Never execute with an unprotected host-only reference. */
    clip_pinned = 1; /* [GC:] Record the root that cleanup must release. */
    fvm_value_number(arguments, 0xff0000); /* Select opaque red paint. */
    if (!checked(fvm_object_call(context, clip, "beginFill", 1, arguments, &output), "beginFill")) /* Start an opaque solid fill. */
        goto cleanup; /* Leave any partial drawing to context cleanup. */
    for (i = 0; i < sizeof(triangle) / sizeof(triangle[0]); i += 2) /* Add all triangle coordinates. */
    { /* Prepare one point for moveTo or lineTo. */
        fvm_value_number(arguments, triangle[i]); /* Set the point's horizontal coordinate. */
        fvm_value_number(arguments + 1, triangle[i + 1]); /* Set its vertical coordinate. */
        if (!checked(fvm_object_call(context, clip, i ? "lineTo" : "moveTo", 2, arguments, &output), "triangle point")) /* Build the path through ordinary native methods. */
            goto cleanup; /* Report a failed geometry operation normally. */
    } /* Finish the closed triangle. */
    if (!checked(fvm_object_call(context, clip, "endFill", 0, 0, &output), "endFill")) /* Commit the completed fill. */
        goto cleanup; /* Do not render an unexpectedly failed drawing operation. */
    if (!checked(fvm_stage_set_quality(context, FVM_STAGE_QUALITY_HIGH), "set_quality")) /* Select antialiased rendering explicitly. */
        goto cleanup; /* Keep failure separate from a disabled graphics backend. */
    memset(pixels, 0, sizeof(pixels)); /* Begin with transparent black output. */
    result = fvm_movieclip_render(context, clip, &surface); /* Render HIGH into caller-owned memory. */
    if (result == FVM_UNSUPPORTED) /* NO_GRAPHICS deliberately has no raster backend. */
    { /* Explain the build requirement without claiming to have rendered pixels. */
        fprintf(stdout, "Enable GRAPHICS_AGG or GRAPHICS_CAIRO to compare edge pixels.\n"); /* Point to the required DEFS selection. */
        exit_code = 0; /* The disabled-backend demonstration is handled deliberately. */
        goto cleanup; /* Release the VM after its synchronous operations return. */
    } /* Finish the explicitly supported headless fallback. */
    if (!checked(result, "render HIGH")) /* Handle all other rendering failures. */
        goto cleanup; /* Avoid reading an unsuccessful output image. */
    high_edges = edge_pixels(pixels); /* Count HIGH's partially covered diagonal edge pixels. */
    if (!checked(fvm_avm1_execute(context, toggle, sizeof(toggle)), "ToggleQuality")) /* Select LOW through the real AVM1 action. */
        goto cleanup; /* Keep the script result visible to the embedding application. */
    if (!checked(fvm_stage_get_quality(context, &quality), "get_quality")) /* Confirm the policy now in effect. */
        goto cleanup; /* Never print an uninitialized enum value. */
    memset(pixels, 0, sizeof(pixels)); /* Remove the previous HIGH image before drawing LOW. */
    if (!checked(fvm_movieclip_render(context, clip, &surface), "render LOW")) /* Apply the selected binary edge coverage. */
        goto cleanup; /* Preserve a failed second rendering as a host error. */
    low_edges = edge_pixels(pixels); /* Count LOW's binary-covered edges after successful rendering. */
    if (quality != FVM_STAGE_QUALITY_LOW || high_edges == 0 || low_edges != 0) /* Check the demonstrated effect through normal control flow. */
    { /* Report a mismatched policy or unexpected renderer result. */
        fprintf(stderr, "The expected HIGH-to-LOW coverage transition was not observed.\n"); /* Explain the failed demonstration. */
        goto cleanup; /* Release the root and VM before returning ordinary failure. */
    } /* Finish validating the intended visual difference. */
    fprintf(stdout, "HIGH partial edges: %u; LOW partial edges: %u; quality: %d\n", high_edges, low_edges, (int)quality); /* Display the effect without backend-specific RGB assumptions. */
    exit_code = 0; /* Both policy selection and rendering completed successfully. */
cleanup: /* Join all partial setup and normal completion paths. */
    if (clip_pinned) /* [GC:] Only release a root that setup successfully acquired. */
    { /* [GC:] Restore host-root ownership before destroying the context. */
        if (!checked(fvm_gc_unpin(context, clip), "gc_unpin")) /* [GC:] Balance the host root without accessing the clip afterwards. */
            exit_code = 1; /* Keep an unexpected root-release error visible. */
    } /* [GC:] Context cleanup owns all remaining managed graph storage. */
    if (context) /* A failed setup may not have created a context. */
        fvm_context_destroy(context); /* Release the clip and the entire managed graph first. */
    if (runtime) /* Runtime creation can also fail independently. */
        fvm_runtime_destroy(runtime); /* Release the runtime only after its context is gone. */
    return exit_code; /* Return an ordinary host status, without assertions or abort. */
} /* End the stage-quality tutorial. */
