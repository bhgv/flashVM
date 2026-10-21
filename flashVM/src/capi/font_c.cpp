/* FlashVM: C embedding API: font.
 * File: src/capi/font_c.cpp.
 */

#include "flashvm/font.h"
#include "font_internal.hpp"
#include "../details/vector.hpp"
#include <cstring>
#include <climits>
#include <new>
#ifdef FVM_HAS_FONTS
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "hb.h"
#include "hb-ft.h"
#include "linebreak.h"
#include "wordbreak.h"
#include "graphemebreak.h"
#include "fribidi.h"
/* Define FVM_Font struct and its representation. */
struct FVM_Font
{
    fvm_detail::vector<unsigned char> bytes;
    FT_Library library;
    FT_Face face;
    hb_font_t *shaper;
    /* Initialize Font and its owned state. */
    FVM_Font() : library(0), face(0), shaper(0)
    {
    }
    /* Release the resources owned by this FVM_Font instance. */
    ~FVM_Font()
    {
        if (shaper)
            hb_font_destroy(shaper);
        if (face)
            FT_Done_Face(face);
        if (library)
            FT_Done_FreeType(library);
    }
};

/* Define FVM_TextRun struct and its representation. */
struct FVM_TextRun
{
    fvm_detail::vector<FVM_ShapedGlyph> glyphs;
};
/* Enforce a bounded Unicode input before passing it to the shaping/break APIs. */
/* Compute and return valid utf8. */
static bool valid_utf8(const char *text, size_t size)
{
    if (size > 4 * 1024 * 1024 || (!text && size))
        return false;
    const unsigned char *s = (const unsigned char *)text;
    for (size_t i = 0; i < size;)
    {
        unsigned int c = s[i++], n = 0, min = 0;
        if (c < 0x80)
            continue;
        if (c >= 0xc2 && c <= 0xdf)
        {
            c &= 31;
            n = 1;
            min = 0x80;
        }
        else if (c >= 0xe0 && c <= 0xef)
        {
            c &= 15;
            n = 2;
            min = 0x800;
        }
        else if (c >= 0xf0 && c <= 0xf4)
        {
            c &= 7;
            n = 3;
            min = 0x10000;
        }
        else
            return false;
        if (n > size - i)
            return false;
        for (unsigned int k = 0; k < n; ++k)
        {
            unsigned int v = s[i++];
            if ((v & 0xc0) != 0x80)
                return false;
            c = (c << 6) | (v & 63);
        }
        if (c < min || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            return false;
    }
    return true;
}

/* Read font. */
extern "C" FVM_Result fvm_font_read(const void *bytes, size_t size, long index, double pixels,
                                    FVM_Font **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!bytes || !size || size > 32 * 1024 * 1024 || index < 0 || !(pixels > 0 && pixels <= 4096))
        return FVM_INVALID_ARGUMENT;
    FVM_Font *font = new (std::nothrow) FVM_Font;
    if (!font)
        return FVM_OUT_OF_MEMORY;
    try
    {
        font->bytes.resize(size);
    }
    catch (...)
    {
        delete font;
        return FVM_OUT_OF_MEMORY;
    }
    std::memcpy(font->bytes.begin(), bytes, size);
    FT_Error err = FT_Init_FreeType(&font->library);
    if (!err)
        err = FT_New_Memory_Face(font->library, font->bytes.begin(), (FT_Long)size, index,
                                 &font->face);
    if (!err)
        err = FT_Set_Char_Size(font->face, 0, (FT_F26Dot6)(pixels * 64 + 0.5), 72, 72);
    if (!err)
        err = FT_Select_Charmap(font->face, FT_ENCODING_UNICODE);
    if (err)
    {
        delete font;
        return err == FT_Err_Out_Of_Memory ? FVM_OUT_OF_MEMORY : FVM_INVALID_ARGUMENT;
    }
    font->shaper = hb_ft_font_create_referenced(font->face);
    if (font->shaper == hb_font_get_empty())
    {
        delete font;
        return FVM_OUT_OF_MEMORY;
    }

    /* Computation / Вычисление и обновление состояния. */
    hb_ft_font_set_load_flags(font->shaper, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    *out = font;
    return FVM_OK;
}

/* Destroy font. */
extern "C" void fvm_font_destroy(FVM_Font *font)
{
    delete font;
}

/* Compute and return font metrics. */
extern "C" FVM_Result fvm_font_metrics(const FVM_Font *font, FVM_FontMetrics *out)
{
    if (!font || !out)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    out->ascent = font->face->size->metrics.ascender / 64.0;
    out->descent = -font->face->size->metrics.descender / 64.0;
    out->line_height = font->face->size->metrics.height / 64.0;
    out->glyph_count = (unsigned int)font->face->num_glyphs;
    return FVM_OK;
}

/* Compute and return font char. */
extern "C" unsigned int fvm_font_char(const FVM_Font *font, unsigned int unicode)
{
    return font && unicode <= 0x10ffff ? FT_Get_Char_Index(font->face, unicode) : 0;
}

/* Define OutlineCopy struct and its representation. */
struct OutlineCopy
{
    FVM_Path *path;
    FVM_Result status;
    bool open;
};
/* Add outline. */
static int add_outline(OutlineCopy *copy, FVM_PathVerb verb, const FT_Vector *a,
                       const FT_Vector *b = 0, const FT_Vector *c = 0)
{
    FVM_PathCommand command;

    /* Computation / Вычисление и обновление состояния. */
    std::memset(&command, 0, sizeof(command));
    command.verb = verb;
    const FT_Vector *v[3] = {a, b, c};
    for (unsigned int i = 0; i < 3; ++i)
        if (v[i])
        {
            command.x[i] = v[i]->x / 64.0;
            command.y[i] = -v[i]->y / 64.0;
        }
    copy->status = fvm_path_add(copy->path, &command);
    return copy->status == FVM_OK ? 0 : 1;
}

/* Compute and return outline move. */
static int outline_move(const FT_Vector *to, void *user)
{
    OutlineCopy *copy = (OutlineCopy *)user;
    if (copy->open && add_outline(copy, FVM_PATH_CLOSE, 0))
        return 1;
    copy->open = true;
    return add_outline(copy, FVM_PATH_MOVE, to);
}

/* Compute and return outline line. */
static int outline_line(const FT_Vector *to, void *user)
{
    return add_outline((OutlineCopy *)user, FVM_PATH_LINE, to);
}

/* Compute and return outline conic. */
static int outline_conic(const FT_Vector *control, const FT_Vector *to, void *user)
{
    return add_outline((OutlineCopy *)user, FVM_PATH_QUADRATIC, control, to);
}

/* Compute and return outline cubic. */
static int outline_cubic(const FT_Vector *a, const FT_Vector *b, const FT_Vector *to, void *user)
{
    return add_outline((OutlineCopy *)user, FVM_PATH_CUBIC, a, b, to);
}

/* Compute and return font glyph. */
extern "C" FVM_Result fvm_font_glyph(FVM_Font *font, unsigned int glyph, FVM_Path **out, double *ax,
                                     double *ay)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!font || glyph >= (unsigned long)font->face->num_glyphs || !ax || !ay)
        return FVM_INVALID_ARGUMENT;
    FT_Error err = FT_Load_Glyph(font->face, glyph, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
    if (err)
        return err == FT_Err_Out_Of_Memory ? FVM_OUT_OF_MEMORY : FVM_ERROR;
    if (font->face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
        return FVM_UNSUPPORTED;

    /* Initialization / Инициализация локального состояния. */
    FVM_Path *path = 0;
    FVM_Result status = fvm_path_create(&path);
    if (status != FVM_OK)
        return status;

    /* Initialization / Инициализация локального состояния. */
    OutlineCopy copy = {path, FVM_OK, false};
    FT_Outline_Funcs callbacks = {outline_move, outline_line, outline_conic, outline_cubic, 0, 0};
    err = FT_Outline_Decompose(&font->face->glyph->outline, &callbacks, &copy);
    if (!err && copy.open)
        err = add_outline(&copy, FVM_PATH_CLOSE, 0);
    if (err)
    {
        fvm_path_destroy(path);
        return copy.status != FVM_OK ? copy.status : FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    *ax = font->face->glyph->advance.x / 64.0;
    *ay = -font->face->glyph->advance.y / 64.0;
    *out = path;
    return FVM_OK;
}

/* Compute and return shape segment. */
static FVM_Result shape_segment(FVM_Font *font, const char *text, size_t size, size_t offset,
                                size_t length, FVM_TextDirection direction, hb_script_t script,
                                const char *language, bool remove_ignorables, unsigned int flags, FVM_TextRun **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if ((flags & ~FVM_FONT_SHAPE_NO_KERNING) || !font || !valid_utf8(text, size) || offset > size || length > size - offset ||
        direction < FVM_TEXT_AUTO || direction > FVM_TEXT_RTL)
        return FVM_INVALID_ARGUMENT;
    hb_buffer_t *buffer = hb_buffer_create();
    if (buffer == hb_buffer_get_empty())
        return FVM_OUT_OF_MEMORY;
    hb_buffer_add_utf8(buffer, text ? text : "", (int)size, (unsigned int)offset, (int)length);
    if (script != HB_SCRIPT_INVALID)
        hb_buffer_set_script(buffer, script);
    if (remove_ignorables)
        hb_buffer_set_flags(buffer, HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES);
    if (direction != FVM_TEXT_AUTO)
        hb_buffer_set_direction(buffer,
                                direction == FVM_TEXT_LTR ? HB_DIRECTION_LTR : HB_DIRECTION_RTL);
    if (language)
        hb_buffer_set_language(buffer, hb_language_from_string(language, -1));

    /* Computation / Вычисление и обновление состояния. */
    hb_buffer_guess_segment_properties(buffer);
    hb_feature_t feature = {HB_TAG('k', 'e', 'r', 'n'), 0, 0, (unsigned int)-1};
    hb_shape(font->shaper, buffer, flags & FVM_FONT_SHAPE_NO_KERNING ? &feature : 0,
             flags & FVM_FONT_SHAPE_NO_KERNING ? 1 : 0);
    if (!hb_buffer_allocation_successful(buffer))
    {
        hb_buffer_destroy(buffer);
        return FVM_OUT_OF_MEMORY;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned int count = 0;
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buffer, &count);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(buffer, 0);
    FVM_TextRun *run = new (std::nothrow) FVM_TextRun;
    if (!run)
    {
        hb_buffer_destroy(buffer);
        return FVM_OUT_OF_MEMORY;
    }
    try
    {
        run->glyphs.resize(count);
    }
    catch (...)
    {
        delete run;
        hb_buffer_destroy(buffer);
        return FVM_OUT_OF_MEMORY;
    }
    for (unsigned int i = 0; i < count; ++i)
    {
        FVM_ShapedGlyph &g = run->glyphs[i];
        g.glyph = info[i].codepoint;
        g.cluster = info[i].cluster;
        g.x_advance = pos[i].x_advance / 64.0;
        g.y_advance = -pos[i].y_advance / 64.0;
        g.x_offset = pos[i].x_offset / 64.0;
        g.y_offset = -pos[i].y_offset / 64.0;
    }
    hb_buffer_destroy(buffer);
    *out = run;
    return FVM_OK;
}

/* Compute and return font shape. */
extern "C" FVM_Result fvm_font_shape(FVM_Font *font, const char *text, size_t size,
                                     FVM_TextDirection direction, const char *language,
                                     FVM_TextRun **out)
{
    return shape_segment(font, text, size, 0, size, direction, HB_SCRIPT_INVALID, language, false, 0,
                         out);
}

/* Shape one owned native run with an explicit, bounded feature selection. */
extern "C" FVM_Result fvm_font_shape_configured(FVM_Font *font, const char *text, size_t size,
                                                FVM_TextDirection direction, const char *language,
                                                unsigned int flags, FVM_TextRun **out)
{
    return shape_segment(font, text, size, 0, size, direction, HB_SCRIPT_INVALID, language,
                         false, flags, out);
}
/* Shape a layout segment using the same selected features as width measurement. */
FVM_Result fvm_font_shape_segment_configured(FVM_Font *font, const char *text, size_t size,
                                             size_t offset, size_t length,
                                             FVM_TextDirection direction, hb_script_t script,
                                             const char *language, unsigned int flags,
                                             FVM_TextRun **out)
{
    return shape_segment(font, text, size, offset, length, direction, script, language, true,
                         flags, out);
}
/* Compute and return font shape segment. */
FVM_Result fvm_font_shape_segment(FVM_Font *font, const char *text, size_t size, size_t offset,
                                  size_t length, FVM_TextDirection direction, hb_script_t script,
                                  const char *language, FVM_TextRun **out)
{
    return shape_segment(font, text, size, offset, length, direction, script, language, true, 0, out);
}

/* Return the number of glyph entries in the shaped run. */
extern "C" size_t fvm_text_run_count(const FVM_TextRun *run)
{
    return run ? run->glyphs.size() : 0;
}

/* Read one glyph placement and its source cluster from the shaped run. */
extern "C" FVM_Result fvm_text_run_get(const FVM_TextRun *run, size_t index, FVM_ShapedGlyph *out)
{
    if (!run || !out || index >= run->glyphs.size())
        return FVM_INVALID_ARGUMENT;
    *out = run->glyphs[index];
    return FVM_OK;
}

/* Release a shaped run and its retained glyph data. */
extern "C" void fvm_text_run_destroy(FVM_TextRun *run)
{
    delete run;
}

/* Export the positioned glyph outlines of a shaped run as one path. */
extern "C" FVM_Result fvm_text_run_path(FVM_Font *font, const FVM_TextRun *run, FVM_Path **out,
                                        double *ax, double *ay)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    if (!font || !run || !ax || !ay)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    FVM_Path *path = 0;
    FVM_Result status = fvm_path_create(&path);
    if (status != FVM_OK)
        return status;
    double x = 0, y = 0;
    for (size_t i = 0; i < run->glyphs.size(); ++i)
    {
        const FVM_ShapedGlyph &g = run->glyphs[i];
        FVM_Path *glyph = 0;
        double gx, gy;
        status = fvm_font_glyph(font, g.glyph, &glyph, &gx, &gy);
        if (status == FVM_OK)
        {
            FVM_Affine place = {1, 0, 0, 1, x + g.x_offset, y + g.y_offset};
            status = fvm_path_append(path, glyph, &place);
        }
        fvm_path_destroy(glyph);
        if (status != FVM_OK)
        {
            fvm_path_destroy(path);
            return status;
        }
        x += g.x_advance;
        y += g.y_advance;
    }

    /* Computation / Вычисление и обновление состояния. */
    *ax = x;
    *ay = y;
    *out = path;
    return FVM_OK;
}

/* Compute and return text breaks. */
extern "C" FVM_Result fvm_text_breaks(const char *text, size_t size, const char *language,
                                      char *line, char *word, char *grapheme)
{
    if (!valid_utf8(text, size) || (!line && !word && !grapheme))
        return FVM_INVALID_ARGUMENT;
    if (size)
    {
        if (line)
            set_linebreaks_utf8((const utf8_t *)text, size, language, line);
        if (word)
            set_wordbreaks_utf8((const utf8_t *)text, size, language, word);
        if (grapheme)
            set_graphemebreaks_utf8((const utf8_t *)text, size, language, grapheme);
    }
    return FVM_OK;
}

/* Compute and return text bidi. */
extern "C" FVM_Result fvm_text_bidi(const unsigned int *logical, size_t count,
                                    FVM_TextDirection *direction, unsigned int *visual, size_t *map,
                                    unsigned char *levels)
{
    if (!direction || (!logical && count) || (!visual && count) || (!map && count) ||
        (!levels && count) || count > 1000000 || *direction < FVM_TEXT_AUTO ||
        *direction > FVM_TEXT_RTL)
        return FVM_INVALID_ARGUMENT;
    if (!count)
    {
        if (*direction == FVM_TEXT_AUTO)
            *direction = FVM_TEXT_LTR;
        return FVM_OK;
    }
    for (size_t i = 0; i < count; ++i)
        if (logical[i] > 0x10ffff || (logical[i] >= 0xd800 && logical[i] <= 0xdfff))
            return FVM_INVALID_ARGUMENT;
    try
    {
        fvm_detail::vector<FriBidiChar> chars(count);
        fvm_detail::vector<FriBidiCharType> types(count);
        fvm_detail::vector<FriBidiBracketType> brackets(count);
        fvm_detail::vector<FriBidiLevel> embedding(count);
        fvm_detail::vector<FriBidiStrIndex> indices(count);
        for (size_t i = 0; i < count; ++i)
        {
            chars[i] = logical[i];
            indices[i] = (FriBidiStrIndex)i;
        }
        fribidi_get_bidi_types(chars.begin(), (FriBidiStrIndex)count, types.begin());
        fribidi_get_bracket_types(chars.begin(), (FriBidiStrIndex)count, types.begin(),
                                  brackets.begin());
        FriBidiParType base = *direction == FVM_TEXT_AUTO  ? FRIBIDI_PAR_ON
                              : *direction == FVM_TEXT_LTR ? FRIBIDI_PAR_LTR
                                                           : FRIBIDI_PAR_RTL;
        if (!fribidi_get_par_embedding_levels_ex(types.begin(), brackets.begin(),
                                                 (FriBidiStrIndex)count, &base, embedding.begin()))
            return FVM_OUT_OF_MEMORY;
        /* Save logical levels before reorder_line resets trailing whitespace. */
        fvm_detail::vector<FriBidiLevel> saved = embedding;
        if (!fribidi_reorder_line(FRIBIDI_FLAG_REORDER_NSM, types.begin(), (FriBidiStrIndex)count,
                                  0, base, embedding.begin(), chars.begin(), indices.begin()))
            return FVM_OUT_OF_MEMORY;
        for (size_t i = 0; i < count; ++i)
        {
            visual[i] = chars[i];
            map[i] = (size_t)indices[i];
            levels[i] = (unsigned char)saved[i];
        }
        *direction = FRIBIDI_IS_RTL(base) ? FVM_TEXT_RTL : FVM_TEXT_LTR;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
#else
/* Preserve a linkable public ABI in NO_FONTS builds. */
/* Read font. */
extern "C" FVM_Result fvm_font_read(const void *, size_t, long, double, FVM_Font **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return FVM_UNSUPPORTED;
}

/* Destroy font. */
extern "C" void fvm_font_destroy(FVM_Font *)
{
}

/* Compute and return font metrics. */
extern "C" FVM_Result fvm_font_metrics(const FVM_Font *, FVM_FontMetrics *)
{
    return FVM_UNSUPPORTED;
}

/* Compute and return font char. */
extern "C" unsigned int fvm_font_char(const FVM_Font *, unsigned int)
{
    return 0;
}

/* Compute and return font glyph. */
extern "C" FVM_Result fvm_font_glyph(FVM_Font *, unsigned int, FVM_Path **out, double *, double *)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return FVM_UNSUPPORTED;
}

/* Compute and return font shape. */
extern "C" FVM_Result fvm_font_shape(FVM_Font *, const char *, size_t, FVM_TextDirection,
                                     const char *, FVM_TextRun **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return FVM_UNSUPPORTED;
}

/* Preserve feature validation and ownership publication when font support is absent. */
extern "C" FVM_Result fvm_font_shape_configured(FVM_Font *, const char *, size_t,
                                                FVM_TextDirection, const char *,
                                                unsigned int flags, FVM_TextRun **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return flags & ~FVM_FONT_SHAPE_NO_KERNING ? FVM_INVALID_ARGUMENT : FVM_UNSUPPORTED;
}

/* Return the number of glyph entries in the shaped run. */
extern "C" size_t fvm_text_run_count(const FVM_TextRun *)
{
    return 0;
}

/* Read one glyph placement and its source cluster from the shaped run. */
extern "C" FVM_Result fvm_text_run_get(const FVM_TextRun *, size_t, FVM_ShapedGlyph *)
{
    return FVM_UNSUPPORTED;
}

/* Release a shaped run and its retained glyph data. */
extern "C" void fvm_text_run_destroy(FVM_TextRun *)
{
}

/* Export the positioned glyph outlines of a shaped run as one path. */
extern "C" FVM_Result fvm_text_run_path(FVM_Font *, const FVM_TextRun *, FVM_Path **out, double *,
                                        double *)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return FVM_UNSUPPORTED;
}

/* Compute and return text breaks. */
extern "C" FVM_Result fvm_text_breaks(const char *, size_t, const char *, char *, char *, char *)
{
    return FVM_UNSUPPORTED;
}

/* Compute and return text bidi. */
extern "C" FVM_Result fvm_text_bidi(const unsigned int *, size_t, FVM_TextDirection *,
                                    unsigned int *, size_t *, unsigned char *)
{
    return FVM_UNSUPPORTED;
}
#endif
