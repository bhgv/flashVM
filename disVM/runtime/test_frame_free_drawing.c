/* Queue drawing independently of frames, then rasterize detached copies after VM teardown. */
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/async.h"
#include "flashvm/timers.h"
#include "monotonic_port.h"
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


/* Supply the same monotonic instant on every sample. Owner polling, callback
 * drawing and rasterization below must not require clock or timeline progress. */
int fvm_port_monotonic_ms(double *value)
{
    *value = 1000;
    return 1;
}


/* Observe real captured contours without retaining a path or entering the VM.
 * userdata owns a counter; all remaining parameters are borrowed values. */
static FVM_Result count_paths(void *userdata, const FVM_Path *path, FVM_RasterColor color,
                             double width, int even_odd)
{
    unsigned int *count = userdata;
    (void)path;
    (void)color;
    (void)width;
    (void)even_odd;
    ++*count;
    return FVM_OK;
}


/* Mutate drawing only on the callback's context owner. The queued clip argument
 * is rooted by the scheduler; no raster thread or foreign heap receives it. */
static FVM_Result draw_posted(FVM_Context *context, FVM_Object *receiver, unsigned int count,
                             const FVM_Value *arguments, FVM_Value *result, void *userdata)
{
    (void)receiver;
    (void)userdata;
    fvm_value_undefined(result);
    if (count != 1 || arguments[0].type != FVM_VALUE_OBJECT || !shape(context, arguments[0].object))
    {
        return FVM_ERROR;
    }
    return FVM_OK;
}


/* Post a real drawing callback, collect pending roots, and poll once at the
 * frozen clock. Before polling the clip has no contours; afterwards it has
 * actual paths. This is an existing runtime API oracle, not asinc lowering. */
static int post_shape(FVM_Context *context, FVM_Object *clip)
{
    FVM_Object *function = NULL;
    FVM_Value argument;
    FVM_Affine identity = {1, 0, 0, 1, 0, 0};
    unsigned int count = 0;
    unsigned int delivered = 0;
    unsigned long id = 0;
    fvm_value_undefined(&argument);
    argument.type = FVM_VALUE_OBJECT;
    argument.object = clip;
    if (fvm_async_install(context) != FVM_OK ||
        fvm_function_create_native(context, draw_posted, NULL, NULL, &function) != FVM_OK ||
        fvm_async_post(context, function, 1, &argument, &id) != FVM_OK || !id ||
        fvm_gc_collect(context, 0) != FVM_OK ||
        fvm_movieclip_capture_paths(context, clip, &identity, count_paths, &count) != FVM_OK || count != 0 ||
        fvm_timers_poll(context, &delivered) != FVM_OK || delivered != 1 ||
        fvm_movieclip_capture_paths(context, clip, &identity, count_paths, &count) != FVM_OK || count == 0)
    {
        return 0;
    }
    return 1;
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
        !post_shape(context, root) || fvm_movieclip_render(context, root, &oracle) != FVM_OK ||
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
    puts("PASS frame-free drawing: frozen clock/no frame advance, rooted callback, actual paths and exact detached RGBA after VM destruction");
    return 0;
}
