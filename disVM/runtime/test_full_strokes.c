/* Direct full-frame rendering must retain stroke and fill-rule metadata. */
#include "editor_raster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Never hide operation evaluation in Release. label identifies the active invariant. */
static void require(int valid, const char *label)
{
    if (!valid)
    {
        fprintf(stderr, "FAIL full vector strokes: %s\n", label);
        exit(1);
    }
}


/* Append one borrowed scalar command, reporting allocation/geometry errors. */
static void add(FVM_Path *path, FVM_PathVerb verb, double x, double y, double endx, double endy)
{
    FVM_PathCommand command;
    memset(&command, 0, sizeof(command));
    command.verb = verb;
    command.x[0] = x;
    command.y[0] = y;
    command.x[1] = endx;
    command.y[1] = endy;
    require(fvm_path_add(path, &command) == FVM_OK, "append independent path");
}


/* Compare the public full-frame branch with an independently drawn clipped
 * surface. path remains caller-owned; this function owns the frame. No tile/
 * incremental helper is called. Unclipped pixels match exactly; cropped AGG
 * edges allow one channel level from translated antialias quantization. */
static void check(FVM_Path *path, double width, int even_odd, int clipped)
{
    unsigned char expected[96 * 96 * 4];
    FVM_RasterColor blue = {20, 60, 190, 255};
    FVM_RasterSurface oracle = {expected, 96, 96, 96 * 4};
    const FVM_RasterSurface *actual;
    DVM_EditorFrame *frame = dvm_editor_frame_create(96, 96);
    unsigned int x;
    unsigned int y;
    unsigned int colored = 0;
    int left = clipped ? 21 : 0;
    int top = clipped ? 19 : 0;
    int right = clipped ? 75 : 96;
    int bottom = clipped ? 78 : 96;
    memset(expected, 255, sizeof(expected));
    require(frame != NULL, "create detached full frame");
    require(dvm_editor_frame_command(frame, path, blue, width, even_odd, left, top, right, bottom), "copy style-bearing path");
    require(fvm_path_draw(path, &oracle, blue, width, even_odd) == FVM_OK, "independent style-bearing oracle");
    for (y = 0; y < 96; y++)
    {
        for (x = 0; x < 96; x++)
        {
            if ((int)x < left || (int)x >= right || (int)y < top || (int)y >= bottom)
            {
                memset(expected + (y * 96 + x) * 4, 255, 4);
            }
        }
    }
    require(dvm_editor_frame_render(frame) == FVM_OK, "full-frame raster");
    actual = dvm_editor_frame_surface(frame);
    for (y = 0; y < 96; y++)
    {
        for (x = 0; x < 96; x++)
        {
            size_t at = (y * 96 + x) * 4;
            colored += actual->pixels[at + 2] > actual->pixels[at] + 40;
        }
    }
    require(colored > 100, "committed open stroke remains visible");
    if (memcmp(actual->pixels, expected, sizeof(expected)))
    {
        unsigned int differences = 0;
        unsigned int maximum = 0;
        size_t at;
        for (at = 0; at < sizeof(expected); at++)
        {
            unsigned int delta = actual->pixels[at] > expected[at]
                ? actual->pixels[at] - expected[at] : expected[at] - actual->pixels[at];
            differences += delta != 0;
            if (delta > maximum)
            {
                maximum = delta;
            }
        }
        fprintf(stderr, "case width=%g even_odd=%d clipped=%d differences=%u maximum=%u\n",
                width, even_odd, clipped, differences, maximum);
        require(clipped && maximum <= 1 && differences < 96,
                "stroke width and fill rule survive full fallback");
    }
    dvm_editor_frame_destroy(frame);
}


/* XOR a borrowed path twice over a coloured base. Full and dirty-tile paths
 * must invert destination RGB and restore exact base bytes, never brush-stamp. */
static void check_xor(FVM_Path *path, int tiled)
{
    DVM_EditorFrame *base = dvm_editor_frame_create(96, 96);
    DVM_EditorFrame *preview = dvm_editor_frame_create(96, 96);
    FVM_RasterColor color = {20, 60, 190, 255};
    const FVM_RasterSurface *a;
    const FVM_RasterSurface *b;
    unsigned int changed = 0;
    unsigned int i;
    require(base && preview, "XOR frame allocation");
    require(dvm_editor_frame_command(base, path, color, 9, 0, 0, 0, 96, 96), "XOR coloured baseline");
    if (tiled)
    {
        require(dvm_editor_frame_incremental(base, NULL), "tiled independent baseline");
    }
    require(dvm_editor_frame_render(base) == FVM_OK, "baseline rasterization");
    require(dvm_editor_frame_command(preview, path, color, 9, 0, 0, 0, 96, 96) &&
            dvm_editor_frame_xor(preview, path, 1, 0, 0, 96, 96), "XOR copied overlay");
    if (tiled)
    {
        require(dvm_editor_frame_incremental(preview, base), "XOR dirty tile admission");
    }
    require(dvm_editor_frame_render(preview) == FVM_OK, "XOR rasterization");
    a = dvm_editor_frame_surface(base);
    b = dvm_editor_frame_surface(preview);
    for (i = 0; i < 96 * 96; ++i)
    {
        if (memcmp(a->pixels + i * 4, b->pixels + i * 4, 3))
        {
            unsigned int c;
            ++changed;
            for (c = 0; c < 3; ++c)
            {
                if ((a->pixels[i * 4 + c] ^ b->pixels[i * 4 + c]) != 255)
                {
                    fprintf(stderr, "XOR mismatch tiled=%d pixel=%u channel=%u base=%u preview=%u\n", tiled, i, c,
                            a->pixels[i * 4 + c], b->pixels[i * 4 + c]);
                    require(0, "XOR destination, not chosen colour");
                }
            }
        }
    }
    require(changed > 40, "XOR outline is visible");
    dvm_editor_frame_destroy(preview);
    preview = dvm_editor_frame_create(96, 96);
    require(preview && dvm_editor_frame_command(preview, path, color, 9, 0, 0, 0, 96, 96) &&
            dvm_editor_frame_xor(preview, path, 1, 0, 0, 96, 96) &&
            dvm_editor_frame_xor(preview, path, 1, 0, 0, 96, 96), "double XOR");
    if (tiled)
    {
        require(dvm_editor_frame_incremental(preview, NULL), "double XOR tiled admission");
    }
    require(dvm_editor_frame_render(preview) == FVM_OK &&
            !memcmp(a->pixels, dvm_editor_frame_surface(preview)->pixels, 96 * 96 * 4), "XOR exactly reversible");
    dvm_editor_frame_destroy(preview);
    dvm_editor_frame_destroy(base);
}


/* Exercise a two-node line and a smoothed Pen path, then a compound hole. */
int main(void)
{
    FVM_Path *path = NULL;
    require(fvm_path_create(&path) == FVM_OK, "line allocation");
    add(path, FVM_PATH_MOVE, 8, 20, 0, 0);
    add(path, FVM_PATH_LINE, 86, 70, 0, 0);
    check_xor(path, 0);
    check_xor(path, 1);
    check(path, 9, 0, 0);
    check(path, 9, 0, 1);
    fvm_path_destroy(path);
    require(fvm_path_create(&path) == FVM_OK, "pen allocation");
    add(path, FVM_PATH_MOVE, 8, 60, 0, 0);
    add(path, FVM_PATH_QUADRATIC, 30, 4, 50, 55);
    add(path, FVM_PATH_LINE, 87, 20, 0, 0);
    check(path, 7, 0, 0);
    check(path, 7, 0, 1);
    fvm_path_destroy(path);
    require(fvm_path_create(&path) == FVM_OK, "hole allocation");
    add(path, FVM_PATH_MOVE, 8, 8, 0, 0);
    add(path, FVM_PATH_LINE, 88, 8, 0, 0);
    add(path, FVM_PATH_LINE, 88, 88, 0, 0);
    add(path, FVM_PATH_LINE, 8, 88, 0, 0);
    add(path, FVM_PATH_CLOSE, 0, 0, 0, 0);
    add(path, FVM_PATH_MOVE, 28, 28, 0, 0);
    add(path, FVM_PATH_LINE, 68, 28, 0, 0);
    add(path, FVM_PATH_LINE, 68, 68, 0, 0);
    add(path, FVM_PATH_LINE, 28, 68, 0, 0);
    add(path, FVM_PATH_CLOSE, 0, 0, 0, 0);
    check(path, 0, 1, 0);
    check(path, 0, 1, 1);
    fvm_path_destroy(path);
    puts("PASS full vector strokes: committed line, quadratic Pen and compound fill rule");
    return 0;
}
