/* Real native pixel parity between owner rendering and a detached queued frame. */
#include "editor_raster.h"
#include "flashvm/render_queue.h"
#include <stdio.h>
#include <string.h>

#define TEST_EXTENT 24U /* Small surface with interior pixels away from antialiased edges. */


/* Create a filled rectangle as owned contours, returning null on any construction failure. */
static FVM_Path *rectangle(double start, double end)
{
    FVM_Path *path = NULL;
    FVM_PathCommand command;
    unsigned int index;
    memset(&command, 0, sizeof(command));
    if (fvm_path_create(&path) != FVM_OK)
    {
        return NULL;
    }
    for (index = 0; index < 5; index++)
    {
        command.verb = index == 4 ? FVM_PATH_CLOSE : index ? FVM_PATH_LINE : FVM_PATH_MOVE;
        command.x[0] = index == 1 || index == 2 ? end : start;
        command.y[0] = index == 2 || index == 3 ? end : start;
        if (fvm_path_add(path, &command) != FVM_OK)
        {
            fvm_path_destroy(path);
            return NULL;
        }
    }
    return path;
}


/* Prepare two independent painter-ordered contours; no VM or font survives in the frame. */
static DVM_EditorFrame *make_frame(void)
{
    DVM_EditorFrame *frame = dvm_editor_frame_create(TEST_EXTENT, TEST_EXTENT);
    FVM_RasterColor blue = {0, 0, 255, 255};
    FVM_RasterColor red = {255, 0, 0, 128};
    FVM_Path *path = rectangle(0, TEST_EXTENT);
    if (!frame || !path || !dvm_editor_frame_path(frame, path, blue))
    {
        fvm_path_destroy(path);
        dvm_editor_frame_destroy(frame);
        return NULL;
    }
    path = rectangle(4, 20);
    if (!path || !dvm_editor_frame_path(frame, path, red))
    {
        fvm_path_destroy(path);
        dvm_editor_frame_destroy(frame);
        return NULL;
    }
    return frame;
}


/* Compare all real RGBA bytes and independently inspect opaque and blended interior pixels.
 * Return process failure on any queue, native raster or ownership error. */
int main(void)
{
    DVM_EditorFrame *reference = make_frame();
    DVM_EditorFrame *candidate = make_frame();
    FVM_RenderQueue *queue = NULL;
    const FVM_RasterSurface *surface;
    const unsigned char *overlap;
    FVM_Result rendered = FVM_ERROR;
    void *completed = NULL;
    int ok = 0;
    if (!reference || !candidate || dvm_editor_frame_create(0, 1) ||
        dvm_editor_frame_create(DVM_EDITOR_FRAME_EXTENT + 1, 1))
    {
        goto done;
    }
    if (dvm_editor_frame_render(reference) != FVM_OK ||
        fvm_render_queue_create(1, dvm_editor_frame_render, dvm_editor_frame_destroy, &queue) != FVM_OK ||
        fvm_render_queue_submit(queue, candidate) != FVM_OK)
    {
        goto done;
    }
    candidate = NULL; /* Queue now owns all candidate contours and pixels. */
    if (fvm_render_queue_take(queue, &completed, &rendered, 1) != FVM_OK || rendered != FVM_OK)
    {
        goto done;
    }
    surface = dvm_editor_frame_surface(completed);
    overlap = surface->pixels + 8 * surface->stride + 8 * 4;
    ok = !memcmp(surface->pixels, dvm_editor_frame_surface(reference)->pixels, TEST_EXTENT * TEST_EXTENT * 4) &&
         surface->pixels[0] == 0 && surface->pixels[1] == 0 && surface->pixels[2] == 255 && overlap[0] >= 127 &&
         overlap[0] <= 128 && overlap[1] == 0 && overlap[2] >= 127 && overlap[2] <= 128 && overlap[3] == 255;
done:
    fvm_render_queue_destroy(queue);
    dvm_editor_frame_destroy(completed);
    dvm_editor_frame_destroy(candidate);
    dvm_editor_frame_destroy(reference);
    if (!ok)
    {
        fprintf(stderr, "detached editor raster parity/ownership failed\n");
        return 1;
    }
    puts("PASS detached editor raster: real RGBA parity, painter order, alpha and native ownership");
    return 0;
}
