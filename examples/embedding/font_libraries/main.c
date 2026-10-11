/* C89 embedding example: OpenType shaping -> independent vector contours ->
 * transform/control-point edit -> AGG/Cairo fill, border and image mask.
 * Uses the CC0 geometric fixture so it runs without installed system fonts. */
#include "flashvm/font.h"
#include "../../../tests/fixtures/font_data.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
    FVM_Font *font = 0;
    FVM_TextRun *run = 0;
    FVM_Path *path = 0;
    FVM_PathBounds bounds;
    FVM_PathCommand first;
    FVM_Affine placement = {1, 0, 0.2, 1, 10, 42};
    double advance_x, advance_y;
    unsigned char pixels[64 * 64 * 4], image[64 * 64 * 4];
    FVM_RasterSurface surface = {0, 64, 64, 64 * 4}, source = {0, 64, 64, 64 * 4};
    FVM_RasterColor fill = {30, 120, 220, 255}, border = {255, 255, 255, 255};
    FVM_Result result;
    size_t count;
    unsigned int x, y;
    int exit_code = 1;
    surface.pixels = pixels;
    source.pixels = image;
    if (fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 32, &font) != FVM_OK)
        goto done;
    if (fvm_font_shape(font, "AV fi", 5, FVM_TEXT_LTR, "en", &run) != FVM_OK)
        goto done;
    count = fvm_text_run_count(run);
    if (fvm_text_run_path(font, run, &path, &advance_x, &advance_y) != FVM_OK)
        goto done;
    /* The exported path owns its geometry after font/run destruction. */
    fvm_font_destroy(font);
    font = 0;
    fvm_text_run_destroy(run);
    run = 0;
    if (fvm_path_transform(path, &placement) != FVM_OK)
        goto done;
    if (fvm_path_get(path, 0, &first) != FVM_OK)
        goto done;
    if (fvm_path_set_point(path, 0, 0, first.x[0] + 0.5, first.y[0]) != FVM_OK)
        goto done;
    if (fvm_path_bounds(path, &bounds) != FVM_OK)
        goto done;
    memset(pixels, 0, sizeof(pixels));
    result = fvm_path_draw(path, &surface, fill, 0, 0);
    if (result == FVM_UNSUPPORTED)
    {
        puts("Vector export passed; raster backend disabled.");
        exit_code = 0;
        goto done;
    }
    if (result != FVM_OK || fvm_path_draw(path, &surface, border, 1, 0) != FVM_OK)
        goto done;
    for (y = 0; y < 64; ++y)
        for (x = 0; x < 64; ++x)
        {
            size_t i = ((size_t)y * 64 + x) * 4;
            image[i] = (unsigned char)(x * 4);
            image[i + 1] = (unsigned char)(y * 4);
            image[i + 2] = 100;
            image[i + 3] = 255;
        }
    if (fvm_path_mask(path, &surface, &source, 0) != FVM_OK)
        goto done;
    printf("%lu glyphs; advance %.3f; vector bounds %.3f %.3f %.3f %.3f; fill/stroke/mask passed\n",
           (unsigned long)count, advance_x, bounds.x_min, bounds.y_min, bounds.x_max, bounds.y_max);
    exit_code = 0;
done:
    fvm_path_destroy(path);
    fvm_text_run_destroy(run);
    fvm_font_destroy(font);
    return exit_code;
}
