#include "flashvm/text_layout.h"
#include "../../../tests/fixtures/font_data.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
#ifdef FVM_HAS_FONTS
    const char text[] = "AV fi \xd7\x90\xd7\x91\n\xd8\xa8\xd8\xa8\xd8\xa8";
    FVM_Font *font = 0;
    FVM_TextLayout *layout = 0;
    FVM_TextLayoutOptions options = {120, 48, FVM_TEXT_AUTO, FVM_TEXT_ALIGN_START, 0};
    FVM_TextLayoutMetrics metrics;
    FVM_TextLayoutLine line;
    FVM_Path *path = 0, *letter = 0;
    FVM_Affine skew = {1, 0, 0.2, 1, 0, 0};
    unsigned char pixels[192 * 160 * 4];
    FVM_RasterSurface surface = {0, 192, 160, 768};
    FVM_RasterColor color = {30, 90, 220, 255};
    size_t i;
    surface.pixels = pixels;
    assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    assert(fvm_text_layout_create(font, text, strlen(text), &options, &layout) == FVM_OK);
    fvm_font_destroy(font);
    assert(fvm_text_layout_metrics(layout, &metrics) == FVM_OK);
    printf("lines=%lu glyphs=%lu advance=%.2f height=%.2f\n", (unsigned long)metrics.line_count,
           (unsigned long)metrics.glyph_count, metrics.advance_width, metrics.height);
    for (i = 0; i < metrics.line_count; ++i)
    {
        assert(fvm_text_layout_line(layout, i, &line) == FVM_OK);
        printf("line %lu: bytes [%lu,%lu), baseline %.2f, direction %s\n", (unsigned long)i,
               (unsigned long)line.byte_start, (unsigned long)line.byte_end, line.baseline,
               line.direction == FVM_TEXT_RTL ? "RTL" : "LTR");
    }
    assert(fvm_text_layout_path(layout, &path) == FVM_OK);
    assert(fvm_text_layout_glyph_path(layout, 0, &letter) == FVM_OK);
    fvm_text_layout_destroy(layout);
    /* Both the block and its first glyph are now independent editable vectors. */
    assert(fvm_path_transform(letter, &skew) == FVM_OK);
    memset(pixels, 0, sizeof(pixels));
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
    assert(fvm_path_draw(path, &surface, color, 0, 0) == FVM_OK);
    assert(fvm_path_draw(letter, &surface, color, 1, 0) == FVM_OK);
#else
    assert(fvm_path_draw(path, &surface, color, 0, 0) == FVM_UNSUPPORTED);
#endif
    fvm_path_destroy(letter);
    fvm_path_destroy(path);
#else
    puts("Text layout requires FONTS in DEFS.txt.");
#endif
    return 0;
}
