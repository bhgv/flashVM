/* Compare dirty parallel tiles against independent full-surface native drawing. */
#include "editor_raster.h"
#include "flashvm/render_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_WIDTH 256U  /* Four tile columns expose untouched and shared edges. */
#define TEST_HEIGHT 128U /* Two rows exercise multiple native raster partitions. */

/* Create a closed rectangle with copied coordinates; caller owns the result. */
static FVM_Path *rectangle(double x, double y, double width, double height)
{
    FVM_Path *path = NULL;
    FVM_PathCommand command;
    unsigned int index;
    memset(&command, 0, sizeof(command));
    if (fvm_path_create(&path) != FVM_OK)
    {
        return NULL;
    }
    for (index = 0; index < 5; ++index)
    {
        command.verb = index == 4 ? FVM_PATH_CLOSE : index ? FVM_PATH_LINE : FVM_PATH_MOVE;
        command.x[0] = x + (index == 1 || index == 2 ? width : 0);
        command.y[0] = y + (index == 2 || index == 3 ? height : 0);
        if (fvm_path_add(path, &command) != FVM_OK)
        {
            fvm_path_destroy(path);
            return NULL;
        }
    }
    return path;
}

/* Append one native command and independently draw the same borrowed path into
 * the complete oracle surface. Return zero on failure; no path escapes the call. */
static int command(DVM_EditorFrame *frame,
                   FVM_RasterSurface *oracle,
                   double x,
                   double y,
                   double width,
                   double height,
                   FVM_RasterColor color,
                   double stroke)
{
    FVM_Path *path = rectangle(x, y, width, height);
    int ok = path && dvm_editor_frame_command(frame, path, color, stroke, 0, 0, 0, TEST_WIDTH, TEST_HEIGHT) &&
             fvm_path_draw(path, oracle, color, stroke, 0) == FVM_OK;
    fvm_path_destroy(path);
    return ok;
}

/* Generate a small changing object behind an unchanged translucent foreground.
 * Reference pixels use the ordinary synchronous rasterizer, never tile helpers. */
static DVM_EditorFrame *scene(unsigned int step, FVM_RasterSurface *oracle)
{
    DVM_EditorFrame *frame = dvm_editor_frame_create(TEST_WIDTH, TEST_HEIGHT);
    FVM_RasterColor blue = {31, 72, 190, 255};
    FVM_RasterColor red = {211, 29, 19, 128};
    FVM_RasterColor green = {20, 190, 60, 143};
    int ok;
    memset(oracle->pixels, 255, oracle->stride * oracle->height);
    ok = frame && command(frame, oracle, 5.25, 67.5, 230, 43, blue, 1.5);
    if (step != 3)
    {
        ok = ok && command(frame, oracle, step == 2 ? 72.25 : 12.25 + step * 8, 11.5, 18, 20, red, 0);
    }
    ok = ok && command(frame, oracle, 26.5, 8.25, 24, 35, green, 0);
    if (!ok)
    {
        dvm_editor_frame_destroy(frame);
        return NULL;
    }
    return frame;
}

/* Count admitted damage independently through the public native-frame contract. */
static unsigned int damage(const DVM_EditorFrame *frame)
{
    unsigned int index;
    unsigned int count = 0;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    for (index = 0; index < 8; ++index)
    {
        count += dvm_editor_frame_damage(frame, index, &x, &y, &width, &height) != 0;
    }
    return count;
}

/* A single text-like path retains distant contours and appends one local glyph.
 * Only the appended contour may cause damage, not the complete text bounds. */
static int appended_contour(void)
{
    DVM_EditorFrame *prior = dvm_editor_frame_create(TEST_WIDTH, TEST_HEIGHT);
    DVM_EditorFrame *next = dvm_editor_frame_create(TEST_WIDTH, TEST_HEIGHT);
    FVM_Path *path = rectangle(150, 70, 60, 20);
    FVM_Path *added = rectangle(12, 12, 8, 15);
    FVM_RasterColor ink = {23, 47, 65, 255};
    int ok = prior && next && path && added &&
             dvm_editor_frame_command(prior, path, ink, 0, 0, 0, 0, TEST_WIDTH, TEST_HEIGHT) &&
             dvm_editor_frame_incremental(prior, NULL) && dvm_editor_frame_render(prior) == FVM_OK &&
             fvm_path_append(path, added, NULL) == FVM_OK &&
             dvm_editor_frame_command(next, path, ink, 0, 0, 0, 0, TEST_WIDTH, TEST_HEIGHT) &&
             dvm_editor_frame_incremental(next, prior) && damage(next) == 1 && dvm_editor_frame_render(next) == FVM_OK;
    if (ok)
    {
        const FVM_RasterSurface *a = dvm_editor_frame_surface(prior);
        const FVM_RasterSurface *b = dvm_editor_frame_surface(next);
        unsigned int row;
        for (row = 64; row < TEST_HEIGHT && ok; ++row)
        {
            ok = memcmp(a->pixels + row * a->stride, b->pixels + row * b->stride, a->stride) == 0;
        }
        ok = ok && b->pixels[16 * b->stride + 16 * 4] == ink.r;
    }
    fvm_path_destroy(path);
    fvm_path_destroy(added);
    dvm_editor_frame_destroy(prior);
    dvm_editor_frame_destroy(next);
    return ok;
}

/* Copy completed damage into a padded owner buffer and compare each actual pixel
 * with the full oracle. Validate counters and every untouched padding byte. */
static int present_checked(const DVM_EditorFrame *frame,
                           FVM_RasterSurface *destination,
                           const FVM_RasterSurface *oracle,
                           int first)
{
    unsigned int row;
    size_t copied = 0;
    if (!dvm_editor_frame_present(frame, destination, first, &copied) ||
        copied + dvm_editor_frame_baseline_bytes(frame) != (size_t)oracle->width * oracle->height * 4U)
    {
        return 0;
    }
    for (row = 0; row < oracle->height; ++row)
    {
        size_t byte;
        if (memcmp(destination->pixels + row * destination->stride,
                   oracle->pixels + row * oracle->stride,
                   oracle->width * 4U))
        {
            return 0;
        }
        for (byte = oracle->width * 4U; byte < destination->stride; ++byte)
        {
            if (destination->pixels[row * destination->stride + byte] != 0xa5)
            {
                return 0;
            }
        }
    }
    return 1;
}

/* Partial edge tiles must preserve padding and invalid inputs must not write.
 * Destroy the prior snapshot before rendering its copied successor. */
static int edge_transfers(void)
{
    FVM_RasterSurface oracle = {NULL, 130, 70, 520};
    FVM_RasterSurface destination = {NULL, 130, 70, 536};
    DVM_EditorFrame *prior = dvm_editor_frame_create(130, 70);
    DVM_EditorFrame *next = dvm_editor_frame_create(130, 70);
    FVM_RasterColor ink = {20, 40, 80, 255};
    size_t copied = 99;
    int ok;
    oracle.pixels = malloc(oracle.stride * oracle.height);
    destination.pixels = malloc(destination.stride * destination.height);
    ok = oracle.pixels && destination.pixels && prior && next;
    if (ok)
    {
        memset(oracle.pixels, 255, oracle.stride * oracle.height);
        memset(destination.pixels, 0xa5, destination.stride * destination.height);
        ok = dvm_editor_frame_incremental(prior, NULL) && dvm_editor_frame_render(prior) == FVM_OK &&
             present_checked(prior, &destination, &oracle, 1) && command(next, &oracle, 129, 69, 1, 1, ink, 0) &&
             dvm_editor_frame_incremental(next, prior);
    }
    dvm_editor_frame_destroy(prior);
    ok = ok && dvm_editor_frame_render(next) == FVM_OK && present_checked(next, &destination, &oracle, 0);
    if (ok)
    {
        FVM_RasterSurface invalid = destination;
        unsigned char before = destination.pixels[0];
        invalid.stride = 1;
        ok = !dvm_editor_frame_present(next, &invalid, 0, &copied) && copied == 0 && destination.pixels[0] == before;
    }
    dvm_editor_frame_destroy(next);
    free(oracle.pixels);
    free(destination.pixels);
    return ok;
}

/* Two jobs prepared against A can complete after a different image B was shown.
 * C's tile mask is not a delta from B. Detect that exact baseline mismatch and
 * force full presentation, including the zero-damage C==A case. */
static int stale_presentation(void)
{
    FVM_RasterSurface oracle = {NULL, TEST_WIDTH, TEST_HEIGHT, TEST_WIDTH * 4U};
    FVM_RasterSurface destination = oracle;
    DVM_EditorFrame *a = dvm_editor_frame_create(TEST_WIDTH, TEST_HEIGHT);
    DVM_EditorFrame *b = dvm_editor_frame_create(TEST_WIDTH, TEST_HEIGHT);
    DVM_EditorFrame *c = dvm_editor_frame_create(TEST_WIDTH, TEST_HEIGHT);
    FVM_RasterColor ink = {200, 30, 40, 255};
    size_t copied = 99;
    int ok;
    oracle.pixels = malloc(oracle.stride * oracle.height);
    destination.pixels = malloc(destination.stride * destination.height);
    ok = oracle.pixels && destination.pixels && a && b && c;
    if (ok)
    {
        memset(oracle.pixels, 255, oracle.stride * oracle.height);
        ok = dvm_editor_frame_incremental(a, NULL) && dvm_editor_frame_render(a) == FVM_OK &&
             dvm_editor_frame_present(a, &destination, 1, NULL) && command(b, &oracle, 10, 10, 20, 20, ink, 0) &&
             dvm_editor_frame_incremental(b, a) && dvm_editor_frame_incremental(c, a) &&
             dvm_editor_frame_render(b) == FVM_OK && dvm_editor_frame_render(c) == FVM_OK &&
             dvm_editor_frame_has_baseline(b, a) && dvm_editor_frame_has_baseline(c, a) &&
             !dvm_editor_frame_has_baseline(c, b) && dvm_editor_frame_present(b, &destination, 0, NULL) &&
             damage(c) == 0 &&
             memcmp(destination.pixels, dvm_editor_frame_surface(c)->pixels, oracle.stride * oracle.height) != 0;
        /* The exact native baseline check selects a complete copy and upload.
         * C's empty damage must not leave B's red rectangle on screen. */
        ok = ok && dvm_editor_frame_present(c, &destination, !dvm_editor_frame_has_baseline(c, b), &copied) &&
             copied == oracle.stride * oracle.height &&
             memcmp(destination.pixels, dvm_editor_frame_surface(c)->pixels, copied) == 0;
    }
    dvm_editor_frame_destroy(a);
    dvm_editor_frame_destroy(b);
    dvm_editor_frame_destroy(c);
    free(oracle.pixels);
    free(destination.pixels);
    return ok;
}

/* Render all scenes through the real queue and compare every output byte.
 * Moved/removed content invalidates old/new bounds; unchanged overlays must be
 * replayed over damaged tiles. The final identical scene must request zero work. */
int main(void)
{
    FVM_RenderQueue *queue = NULL;
    DVM_EditorFrame *prior = NULL;
    DVM_EditorFrame *frame = NULL;
    FVM_RasterSurface oracle;
    FVM_RasterSurface destination;
    unsigned int step;
    int ok = 0;
    oracle.width = TEST_WIDTH;
    oracle.height = TEST_HEIGHT;
    oracle.stride = TEST_WIDTH * 4;
    oracle.pixels = malloc(oracle.stride * oracle.height);
    destination = oracle;
    destination.stride += 12;
    destination.pixels = malloc(destination.stride * destination.height);
    if (destination.pixels)
    {
        memset(destination.pixels, 0xa5, destination.stride * destination.height);
    }
    if (!oracle.pixels || !destination.pixels ||
        fvm_render_queue_create(1, dvm_editor_frame_render, dvm_editor_frame_destroy, &queue) != FVM_OK)
    {
        goto done;
    }
    for (step = 0; step < 5; ++step)
    {
        void *completed = NULL;
        FVM_Result rendered = FVM_ERROR;
        frame = scene(step == 4 ? 3 : step, &oracle);
        if (!frame || !dvm_editor_frame_incremental(frame, prior) || (step == 0 && damage(frame) != 8) ||
            (step == 1 && damage(frame) != 1) || (step == 2 && damage(frame) != 2) ||
            (step == 4 && damage(frame) != 0) || fvm_render_queue_submit(queue, frame) != FVM_OK)
        {
            fprintf(stderr, "damage admission failed at scene %u\n", step);
            goto done;
        }
        frame = NULL;
        if (fvm_render_queue_take(queue, &completed, &rendered, 1) != FVM_OK)
        {
            goto done;
        }
        frame = completed;
        if (rendered != FVM_OK ||
            memcmp(dvm_editor_frame_surface(frame)->pixels, oracle.pixels, oracle.stride * oracle.height))
        {
            fprintf(stderr, "parallel/full raster byte mismatch at scene %u\n", step);
            goto done;
        }
        if (!present_checked(frame, &destination, &oracle, step == 0))
        {
            fprintf(stderr, "dirty transfer or padding failed at scene %u\n", step);
            goto done;
        }
        if (step == 4)
        {
            size_t copied = 99;
            if (!dvm_editor_frame_present(frame, &destination, 0, &copied) || copied != 0)
            {
                fprintf(stderr, "unchanged frame copied presentation pixels\n");
                goto done;
            }
        }
        dvm_editor_frame_destroy(prior);
        prior = frame;
        frame = NULL;
    }
    ok = appended_contour() && edge_transfers() && stale_presentation();
done:
    fvm_render_queue_destroy(queue);
    dvm_editor_frame_destroy(frame);
    dvm_editor_frame_destroy(prior);
    free(oracle.pixels);
    free(destination.pixels);
    if (!ok)
    {
        return 1;
    }
    puts("PASS dirty parallel raster: exact pixels, tile edges, alpha, old/new bounds, removal, zero-work scene");
    return 0;
}
