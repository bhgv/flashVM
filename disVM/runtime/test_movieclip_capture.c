/* Capture real MovieClip geometry, destroy the VM, then draw native copies. */
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "editor_raster.h"
#include <stdio.h>
#include <string.h>

#define TEST_CAPTURE_EXTENT 128U /* Covers multiple dirty tiles and transformed edges. */


/* Copy every contour into the exclusively owner-held frame; no VM operation. */
static FVM_Result capture(void *value, const FVM_Path *path, FVM_RasterColor color, double width, int even_odd)
{
    return dvm_editor_frame_command(value, path, color, width, even_odd, 0, 0, TEST_CAPTURE_EXTENT, TEST_CAPTURE_EXTENT)
               ? FVM_OK
               : FVM_ERROR;
}


/* Verify that callback errors propagate immediately without invoking raster work. */
static FVM_Result reject(void *value, const FVM_Path *path, FVM_RasterColor color, double width, int even_odd)
{
    (void)value;
    (void)path;
    (void)color;
    (void)width;
    (void)even_odd;
    return FVM_ERROR;
}


/* Invoke a standard MovieClip drawing method using copied scalar parameters. */
static int call(FVM_Context *context,
                FVM_Object *clip,
                const char *name,
                unsigned int count,
                double first,
                double second,
                double third)
{
    FVM_Value arguments[3];
    FVM_Value result;
    fvm_value_number(arguments, first);
    fvm_value_number(arguments + 1, second);
    fvm_value_number(arguments + 2, third);
    return fvm_object_call(context, clip, name, count, arguments, &result) == FVM_OK;
}


/* Draw a partially transparent filled/stroked shape crossing native tile edges. */
static int shape(FVM_Context *context, FVM_Object *clip)
{
    return call(context, clip, "lineStyle", 3, 2, 0x1947bf, 100) &&
           call(context, clip, "beginFill", 2, 0xd62919, 65, 0) && call(context, clip, "moveTo", 2, 10.25, 12.5, 0) &&
           call(context, clip, "lineTo", 2, 103.25, 12.5, 0) && call(context, clip, "lineTo", 2, 103.25, 93.5, 0) &&
           call(context, clip, "lineTo", 2, 10.25, 93.5, 0) && call(context, clip, "lineTo", 2, 10.25, 12.5, 0) &&
           call(context, clip, "endFill", 0, 0, 0, 0);
}


/* Compare every byte against ordinary MovieClip rendering. Native commands must
 * survive VM destruction; invalid inputs and sink failures preserve explicit errors. */
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    FVM_Object *root = NULL;
    DVM_EditorFrame *frame = NULL;
    unsigned char pixels[TEST_CAPTURE_EXTENT * TEST_CAPTURE_EXTENT * 4];
    FVM_RasterSurface oracle = {pixels, TEST_CAPTURE_EXTENT, TEST_CAPTURE_EXTENT, TEST_CAPTURE_EXTENT * 4};
    FVM_Affine identity = {1, 0, 0, 1, 0, 0};
    int ok = 0;
    memset(pixels, 255, sizeof(pixels));
    frame = dvm_editor_frame_create(TEST_CAPTURE_EXTENT, TEST_CAPTURE_EXTENT);
    if (!frame || fvm_config_init(&config) != FVM_OK || fvm_runtime_create(&config, &runtime) != FVM_OK ||
        fvm_context_create(runtime, &context) != FVM_OK || fvm_standard_install(context) != FVM_OK ||
        fvm_movieclip_create(context, "capture", 1, &root) != FVM_OK || fvm_gc_pin(context, root) != FVM_OK ||
        !shape(context, root) || fvm_movieclip_render(context, root, &oracle) != FVM_OK ||
        fvm_movieclip_capture_paths(context, root, &identity, capture, frame) != FVM_OK ||
        fvm_movieclip_capture_paths(context, root, &identity, reject, NULL) != FVM_ERROR ||
        fvm_movieclip_capture_paths(context, root, NULL, capture, frame) != FVM_INVALID_ARGUMENT)
    {
        goto done;
    }
    fvm_context_destroy(context);
    context = NULL;
    fvm_runtime_destroy(runtime);
    runtime = NULL;
    if (!dvm_editor_frame_incremental(frame, NULL) || dvm_editor_frame_render(frame) != FVM_OK ||
        memcmp(pixels, dvm_editor_frame_surface(frame)->pixels, sizeof(pixels)))
    {
        goto done;
    }
    ok = 1;
done:
    dvm_editor_frame_destroy(frame);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    if (!ok)
    {
        fputs("MovieClip native capture/pixel/lifetime contract failed\n", stderr);
        return 1;
    }
    puts("PASS MovieClip native capture: exact solid/stroke RGBA after VM destruction, sink errors, argument guards");
    return 0;
}
