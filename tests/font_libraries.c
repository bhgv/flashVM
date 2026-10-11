#include "flashvm/font.h"
#include "fixtures/font_data.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include "hb.h"
#include "linebreak.h"
#include "wordbreak.h"
#include "graphemebreak.h"
#include "fribidi.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

int main(void)
{
    FVM_Font *font = 0;
    FVM_FontMetrics metrics;
    FVM_TextRun *run = 0;
    FVM_ShapedGlyph g;
    FVM_Path *path = 0;
    FVM_PathBounds box;
    double ax, ay;
    unsigned int a, b, i;
    char line[32], word[32], grapheme[32];
    const char text[] = "one two\n";
    const char accent[] = "a\xcc\x81";
    unsigned int logical[] = {65, 32, 0x5d0, 0x5d1, 32, 86};
    unsigned int visual[6];
    size_t map[6];
    unsigned char levels[6];
    FVM_TextDirection direction = FVM_TEXT_AUTO;
    FT_Library library = 0;
    FT_Face face = 0;
    FT_BBox native;
    unsigned char *input_copy;
    hb_buffer_t *empty;
    unsigned int empty_count;
    assert(strcmp(hb_version_string(), "1.8.8") == 0);
    empty = hb_buffer_create();
    empty_count = 123;
    assert(hb_buffer_get_glyph_positions(empty, &empty_count) == 0 && empty_count == 0);
    assert(hb_buffer_allocation_successful(empty));
    hb_buffer_destroy(empty);
    assert(hb_tag_from_string("\xff\xff\xff\xff", 4) == HB_TAG_MAX);
    assert(sizeof(hb_script_t) == sizeof(int));
    assert(unibreak_version == 0x0601);
    assert(strcmp(fribidi_unicode_version, "16.0.0") == 0);
    assert(fvm_font_read(fvm_test_font, 20, 0, 40, &font) == FVM_INVALID_ARGUMENT && !font);
    input_copy = (unsigned char *)malloc(sizeof(fvm_test_font));
    assert(input_copy);
    memcpy(input_copy, fvm_test_font, sizeof(fvm_test_font));
    assert(fvm_font_read(input_copy, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    memset(input_copy, 0, sizeof(fvm_test_font));
    free(input_copy);
    assert(fvm_font_metrics(font, &metrics) == FVM_OK && metrics.glyph_count == 16);
    assert(metrics.ascent == 36 && metrics.descent == 8 && metrics.line_height == 48);
    a = fvm_font_char(font, 'A');
    b = fvm_font_char(font, 'B');
    assert(a == 2 && b == 7);
    assert(fvm_font_char(font, 0x10ffff) == 0);
    assert(fvm_font_glyph(font, b, &path, &ax, &ay) == FVM_OK);
    assert(ax == 24 && ay == 0);
    assert(fvm_path_bounds(path, &box) == FVM_OK);
    assert(fabs(box.x_min - 4) < 0.001 && fabs(box.x_max - 20) < 0.001);
    assert(fabs(box.y_min + 36) < 0.001 && box.y_max == 0);
    /* Independently compare geometry with FreeType's exact extrema API. */
    assert(FT_Init_FreeType(&library) == 0);
    assert(FT_New_Memory_Face(library, fvm_test_font, sizeof(fvm_test_font), 0, &face) == 0);
    assert(FT_Set_Char_Size(face, 0, 40 * 64, 72, 72) == 0);
    assert(FT_Load_Glyph(face, b, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) == 0);
    assert(FT_Outline_Get_BBox(&face->glyph->outline, &native) == 0);
    assert(fabs(box.x_max - native.xMax / 64.0) < 0.001);
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    fvm_path_destroy(path);
    assert(fvm_font_glyph(font, fvm_font_char(font, 0xc1), &path, &ax, &ay) == FVM_OK);
    assert(fvm_path_bounds(path, &box) == FVM_OK && fabs(box.y_min + 36) < 0.001);
    fvm_path_destroy(path);
    assert(fvm_font_shape(font, "fi", 2, FVM_TEXT_LTR, "en", &run) == FVM_OK);
    assert(fvm_text_run_count(run) == 1);
    assert(fvm_text_run_get(run, 0, &g) == FVM_OK && g.glyph == 6 && g.cluster == 0 &&
           g.x_advance == 28);
    assert(fvm_text_run_path(font, run, &path, &ax, &ay) == FVM_OK && ax == 28);
    fvm_path_destroy(path);
    fvm_text_run_destroy(run);
    assert(fvm_font_shape(font, "AV", 2, FVM_TEXT_AUTO, "en", &run) == FVM_OK);
    assert(fvm_text_run_count(run) == 2);
    assert(fvm_text_run_get(run, 0, &g) == FVM_OK && fabs(g.x_advance - 20.8) < 0.02);
    assert(fvm_text_run_path(font, run, &path, &ax, &ay) == FVM_OK && fabs(ax - 44.8) < 0.02);
    assert(fvm_path_bounds(path, &box) == FVM_OK && box.x_max < ax);
    fvm_path_destroy(path);
    fvm_text_run_destroy(run);
    /* Hebrew visual order and UTF-8 byte clusters. */
    assert(fvm_font_shape(font, "\xd7\x90\xd7\x91", 4, FVM_TEXT_RTL, "he", &run) == FVM_OK);
    assert(fvm_text_run_get(run, 0, &g) == FVM_OK && g.glyph == 11 && g.cluster == 2);
    assert(fvm_text_run_get(run, 1, &g) == FVM_OK && g.glyph == 10 && g.cluster == 0);
    fvm_text_run_destroy(run);
    /* Arabic initial/medial/final substitution comes from OpenType shaping. */
    assert(fvm_font_shape(font, "\xd8\xa8\xd8\xa8\xd8\xa8", 6, FVM_TEXT_RTL, "ar", &run) == FVM_OK);
    assert(fvm_text_run_count(run) == 3);
    assert(fvm_text_run_get(run, 0, &g) == FVM_OK && g.glyph == 15);
    assert(fvm_text_run_get(run, 1, &g) == FVM_OK && g.glyph == 14);
    assert(fvm_text_run_get(run, 2, &g) == FVM_OK && g.glyph == 13);
    fvm_text_run_destroy(run);
    assert(fvm_font_shape(font, "\xc0\xaf", 2, FVM_TEXT_AUTO, 0, &run) == FVM_INVALID_ARGUMENT &&
           !run);
    assert(fvm_font_shape(font, 0, 0, FVM_TEXT_AUTO, 0, &run) == FVM_OK &&
           fvm_text_run_count(run) == 0);
    fvm_text_run_destroy(run);
    assert(fvm_text_breaks(text, strlen(text), "en", line, word, grapheme) == FVM_OK);
    assert(line[3] == LINEBREAK_ALLOWBREAK && line[7] == LINEBREAK_MUSTBREAK);
    assert(word[2] == WORDBREAK_BREAK);
    assert(fvm_text_breaks(accent, strlen(accent), 0, line, word, grapheme) == FVM_OK);
    assert(grapheme[0] == GRAPHEMEBREAK_NOBREAK && grapheme[1] == GRAPHEMEBREAK_INSIDEACHAR);
    assert(fvm_text_breaks("\xed\xa0\x80", 3, 0, line, 0, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_text_bidi(logical, 6, &direction, visual, map, levels) == FVM_OK);
    assert(direction == FVM_TEXT_LTR && visual[2] == 0x5d1 && visual[3] == 0x5d0);
    assert(map[2] == 3 && map[3] == 2 && (levels[2] & 1));
    for (i = 0; i < 6; ++i)
        assert(visual[i] == logical[map[i]]);
    fvm_font_destroy(font);
    /* A copied glyph's vector data remains editable after closing its font. */
    assert(fvm_font_read(fvm_test_font, sizeof(fvm_test_font), 0, 40, &font) == FVM_OK);
    assert(fvm_font_glyph(font, a, &path, &ax, &ay) == FVM_OK);
    fvm_font_destroy(font);
    assert(fvm_path_count(path) > 0 && fvm_path_set_point(path, 0, 0, 1, 2) == FVM_OK);
    fvm_path_destroy(path);
    return 0;
}
