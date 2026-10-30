/* Retained native layer cache: compare incremental composition with fresh scenes. */
#include "editor_raster.h"
#include "editor_pointer_gate.h"
#include "flashvm/render_queue.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 256U  /* Four tile columns. */
#define HEIGHT 192U /* Three tile rows. */

/* Create a local closed rectangle and transform it exactly as owner capture does.
 * Caller owns the returned native path; null means allocation failure. */
static FVM_Path *shape(const FVM_Affine *world, unsigned int width, unsigned int height)
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
        command.x[0] = index == 1 || index == 2 ? width : 0;
        command.y[0] = index == 2 || index == 3 ? height : 0;
        if (fvm_path_add(path, &command) != FVM_OK)
        {
            fvm_path_destroy(path);
            return NULL;
        }
    }
    if (fvm_path_transform(path, world) != FVM_OK)
    {
        fvm_path_destroy(path);
        return NULL;
    }
    return path;
}

/* Append one object's real native ink; the child parent is metadata for dirty
 * propagation, while world already contains its owner's composed transform. */
static int object(DVM_EditorFrame *frame,
                  unsigned int id,
                  unsigned int parent,
                  FVM_Affine world,
                  unsigned int width,
                  unsigned int height,
                  FVM_RasterColor color,
                  unsigned int opacity)
{
    FVM_Path *path = shape(&world, width, height);
    int ok = path && dvm_editor_frame_node_begin(frame, id, parent) &&
             dvm_editor_frame_command(frame, path, color, 0, 0, 0, 0, WIDTH, HEIGHT) &&
             dvm_editor_frame_node_end(frame, &world, width, height, opacity);
    fvm_path_destroy(path);
    return ok;
}

/* Change transform/opacity without changing local ink; later steps change content,
 * remove a child, reorder independent layers, and finally repeat an identical scene. */
static DVM_EditorFrame *scene(unsigned int step)
{
    DVM_EditorFrame *frame = dvm_editor_frame_create(WIDTH, HEIGHT);
    FVM_Affine parent = {1, 0, 0, 1, 20, 50};
    FVM_Affine child = {1, 0, 0, 1, 40, 70};
    FVM_Affine overlay = {1, 0, 0, 1, 70, 80};
    FVM_RasterColor red = {220, 20, 10, 255};
    FVM_RasterColor green = {10, 180, 30, 128};
    FVM_RasterColor blue = {10, 30, 220, 255};
    int ok;
    if (step >= 1)
    {
        parent.x0 += 15;
        child.x0 += 15;
    }
    if (step == 3)
    {
        parent.xx = parent.yy = 1.5;
        child.xx = child.yy = 1.5;
        child.x0 = parent.x0 + 30;
        child.y0 = parent.y0 + 30;
    }
    if (step == 4)
    {
        parent.xx = parent.yy = 0;
        parent.xy = -1;
        parent.yx = 1;
        parent.x0 = 160;
        child.xx = child.yy = 0;
        child.xy = -1;
        child.yx = 1;
        child.x0 = 140;
        child.y0 = 70;
    }
    if (step >= 5)
    {
        green.r = 150;
    }
    if (step >= 7)
    {
        ok =
            frame && object(frame, 3, 0, overlay, 60, 15, blue, 1000) && object(frame, 1, 0, parent, 80, 60, red, 1000);
    }
    else
    {
        ok = frame && object(frame, 1, 0, parent, 80, 60, red, step == 2 ? 900 : 1000);
        if (step < 6)
        {
            ok = ok && object(frame, 2, 1, child, 40, 20, green, 1000);
        }
        ok = ok && object(frame, 3, 0, overlay, 60, 15, blue, 1000);
    }
    if (!ok)
    {
        dvm_editor_frame_destroy(frame);
        return NULL;
    }
    return frame;
}

/* Check native FIFO rules independently of SDL availability and actual raster speed. */
static int pointer_rules(void)
{
    return dvm_editor_pointer_waits(1, 0, 0, 0, 0, 0) && !dvm_editor_pointer_waits(1, 1, 0, 0, 0, 1) &&
           !dvm_editor_pointer_waits(1, 1, 0, 0, 1, 0) && dvm_editor_pointer_waits(1, 1, 1, 0, 0, 1) &&
           dvm_editor_pointer_waits(1, 1, 0, 1, 0, 1) && !dvm_editor_pointer_waits(0, 0, 0, 0, 0, 0);
}

/* A retained native command can outlive its original completed frame. Replay must
 * copy its local commands and key; mutation of caller key bytes must not alias it. */
static int replay_lifetime(void)
{
    DVM_EditorFrame *prior = dvm_editor_frame_create(WIDTH, HEIGHT);
    DVM_EditorFrame *next = dvm_editor_frame_create(WIDTH, HEIGHT);
    FVM_Affine world = {1, 0, 0, 1, 30, 40};
    FVM_RasterColor ink = {220, 20, 10, 255};
    FVM_Path *path = shape(&world, 30, 20);
    unsigned char key[3] = {1, 2, 3};
    unsigned int built = 0, reused = 0;
    int ok = prior && next && path && dvm_editor_frame_node_begin(prior, 1, 0) &&
             dvm_editor_frame_node_key(prior, key, sizeof(key)) &&
             dvm_editor_frame_command(prior, path, ink, 0, 0, 0, 0, WIDTH, HEIGHT) &&
             dvm_editor_frame_node_end(prior, &world, 30, 20, 1000) && dvm_editor_frame_incremental(prior, NULL) &&
             dvm_editor_frame_render(prior) == FVM_OK;
    key[0] = 9;
    ok = ok && !dvm_editor_frame_node_matches(prior, 1, key, sizeof(key));
    key[0] = 1;
    world.x0 += 20;
    ok = ok && dvm_editor_frame_node_matches(prior, 1, key, sizeof(key)) && dvm_editor_frame_node_begin(next, 1, 0) &&
         dvm_editor_frame_node_replay(next, prior, 1) && dvm_editor_frame_node_end(next, &world, 30, 20, 900) &&
         dvm_editor_frame_incremental(next, prior);
    dvm_editor_frame_destroy(prior);
    ok = ok && dvm_editor_frame_render(next) == FVM_OK;
    dvm_editor_frame_scene_stats(next, &built, &reused, NULL);
    ok = ok && built == 0 && reused == 1 && dvm_editor_frame_surface(next)->pixels[45 * WIDTH * 4 + 55 * 4] > 200;
    fvm_path_destroy(path);
    dvm_editor_frame_destroy(next);
    return ok;
}

/* Appending a distant glyph patches only its local contour bounds in the object's
 * own bitmap. Retaining the base permits destruction of the displayed snapshot
 * before the candidate worker renders; final RGBA must match a fresh full build. */
static int local_patch(void)
{
    DVM_EditorFrame *prior = dvm_editor_frame_create(WIDTH, HEIGHT);
    DVM_EditorFrame *next = dvm_editor_frame_create(WIDTH, HEIGHT);
    DVM_EditorFrame *fresh = dvm_editor_frame_create(WIDTH, HEIGHT);
    FVM_Affine distant = {1, 0, 0, 1, 150, 70};
    FVM_Affine near = {1, 0, 0, 1, 12, 12};
    FVM_Affine world = {1, 0, 0, 1, 0, 0};
    FVM_RasterColor ink = {20, 40, 80, 255};
    FVM_Path *path = shape(&distant, 60, 20);
    FVM_Path *added = shape(&near, 8, 15);
    int ok = prior && next && fresh && path && added && dvm_editor_frame_node_begin(prior, 1, 0) &&
             dvm_editor_frame_command(prior, path, ink, 0, 0, 0, 0, WIDTH, HEIGHT) &&
             dvm_editor_frame_node_end(prior, &world, 220, 110, 1000) && dvm_editor_frame_incremental(prior, NULL) &&
             dvm_editor_frame_render(prior) == FVM_OK && fvm_path_append(path, added, NULL) == FVM_OK &&
             dvm_editor_frame_node_begin(next, 1, 0) &&
             dvm_editor_frame_command(next, path, ink, 0, 0, 0, 0, WIDTH, HEIGHT) &&
             dvm_editor_frame_node_end(next, &world, 220, 110, 1000) && dvm_editor_frame_incremental(next, prior) &&
             dvm_editor_frame_node_begin(fresh, 1, 0) &&
             dvm_editor_frame_command(fresh, path, ink, 0, 0, 0, 0, WIDTH, HEIGHT) &&
             dvm_editor_frame_node_end(fresh, &world, 220, 110, 1000) && dvm_editor_frame_incremental(fresh, NULL);
    dvm_editor_frame_destroy(prior);
    ok = ok && dvm_editor_frame_render(next) == FVM_OK && dvm_editor_frame_render(fresh) == FVM_OK &&
         dvm_editor_frame_scene_pixels(next) > 0 && dvm_editor_frame_scene_pixels(next) < 1000 &&
         memcmp(dvm_editor_frame_surface(next)->pixels, dvm_editor_frame_surface(fresh)->pixels, WIDTH * HEIGHT * 4) ==
             0;
    fvm_path_destroy(path);
    fvm_path_destroy(added);
    dvm_editor_frame_destroy(next);
    dvm_editor_frame_destroy(fresh);
    return ok;
}

/* Invalid forests and excessive aggregate bitmap budgets fail before any job is
 * submitted. Destroying those candidates must leave retained baselines usable. */
static int invalid_scenes(void)
{
    DVM_EditorFrame *cycle = dvm_editor_frame_create(WIDTH, HEIGHT);
    DVM_EditorFrame *budget = dvm_editor_frame_create(WIDTH, HEIGHT);
    FVM_Affine world = {1, 0, 0, 1, 10, 20};
    FVM_RasterColor ink = {1, 2, 3, 255};
    unsigned int id;
    int ok = cycle && budget && object(cycle, 1, 2, world, 20, 10, ink, 1000) &&
             object(cycle, 2, 1, world, 20, 10, ink, 1000) && !dvm_editor_frame_incremental(cycle, NULL);
    for (id = 1; id <= 4 && ok; ++id)
    {
        ok = object(budget, id, 0, world, DVM_EDITOR_FRAME_EXTENT, DVM_EDITOR_FRAME_EXTENT, ink, 1000);
    }
    ok = ok && !dvm_editor_frame_incremental(budget, NULL);
    dvm_editor_frame_destroy(cycle);
    dvm_editor_frame_destroy(budget);
    return ok;
}

/* Queue replacement/shutdown may release frames on another thread. Both queued
 * transforms share immutable pixels, and the prior owner can disappear first. */
static int queue_lifetime(void)
{
    DVM_EditorFrame *prior = scene(0);
    DVM_EditorFrame *first = scene(1);
    DVM_EditorFrame *last = scene(2);
    FVM_RenderQueue *queue = NULL;
    int ok = prior && first && last && dvm_editor_frame_incremental(prior, NULL) &&
             dvm_editor_frame_render(prior) == FVM_OK && dvm_editor_frame_incremental(first, prior) &&
             dvm_editor_frame_incremental(last, prior) &&
             fvm_render_queue_create(1, dvm_editor_frame_render, dvm_editor_frame_destroy, &queue) == FVM_OK;
    if (ok && fvm_render_queue_submit(queue, first) == FVM_OK)
    {
        first = NULL;
    }
    else
    {
        ok = 0;
    }
    if (ok && fvm_render_queue_submit(queue, last) == FVM_OK)
    {
        last = NULL;
    }
    else
    {
        ok = 0;
    }
    dvm_editor_frame_destroy(prior);
    fvm_render_queue_destroy(queue);
    dvm_editor_frame_destroy(first);
    dvm_editor_frame_destroy(last);
    return ok && dvm_editor_scene_live_images() == 0;
}

/* Fresh composition is an independent damage/cache oracle. Assert that moving,
 * fading, zooming or rotating never rasterizes unchanged local object content. */
int main(void)
{
    DVM_EditorFrame *prior = NULL;
    unsigned int step;
    if (!pointer_rules() || !replay_lifetime() || !local_patch() || !invalid_scenes() || !queue_lifetime())
    {
        fprintf(stderr, "retained capture lifetime/input rule failed\n");
        return 1;
    }
    for (step = 0; step < 9; ++step)
    {
        DVM_EditorFrame *frame = scene(step);
        DVM_EditorFrame *fresh = scene(step);
        unsigned int built = 0, reused = 0;
        int ok =
            frame && fresh && dvm_editor_frame_incremental(frame, prior) && dvm_editor_frame_incremental(fresh, NULL);
        if (step == 5 || step == 6)
        {
            ok = ok && (dvm_editor_frame_node_flags(frame, 1) & DVM_EDITOR_NODE_COMPOSITE);
        }
        ok = ok && dvm_editor_frame_render(frame) == FVM_OK && dvm_editor_frame_render(fresh) == FVM_OK &&
             dvm_editor_frame_node_flags(frame, 1) == 0 && dvm_editor_frame_node_flags(frame, 2) == 0;
        dvm_editor_frame_scene_stats(frame, &built, &reused, NULL);
        ok = ok && memcmp(dvm_editor_frame_surface(frame)->pixels,
                          dvm_editor_frame_surface(fresh)->pixels,
                          WIDTH * HEIGHT * 4) == 0;
        if (step == 0)
        {
            ok = ok && built == 3 && reused == 0;
        }
        if (step >= 1 && step <= 4)
        {
            ok = ok && built == 0 && reused == 3;
        }
        if (step == 5)
        {
            ok = ok && built == 1 && reused == 2;
        }
        if (step >= 6)
        {
            ok = ok && built == 0 && reused == 2;
        }
        if (!ok)
        {
            fprintf(stderr, "retained scene failed step=%u built=%u reused=%u\n", step, built, reused);
            dvm_editor_frame_destroy(frame);
            dvm_editor_frame_destroy(fresh);
            dvm_editor_frame_destroy(prior);
            return 1;
        }
        dvm_editor_frame_destroy(prior);
        dvm_editor_frame_destroy(fresh);
        prior = frame;
    }
    dvm_editor_frame_destroy(prior);
    if (dvm_editor_scene_live_images() != 0)
    {
        fprintf(stderr, "retained images leaked after final frame release\n");
        return 1;
    }
    puts(
        "PASS retained scene: exact fresh/incremental pixels, zero repaint on transforms, local invalidation, removal, order and immutable image lifetime");
    return 0;
}
