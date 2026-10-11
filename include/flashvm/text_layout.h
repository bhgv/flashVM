#ifndef FLASHVM_TEXT_LAYOUT_H
#define FLASHVM_TEXT_LAYOUT_H
#include "flashvm/font.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Horizontal, single-font layout. Whitespace is preserved. Width zero disables
     * wrapping; an indivisible oversized grapheme is allowed to exceed the width.
     * START follows the resolved paragraph direction. Coordinates are x right/y down. */
    typedef struct FVM_TextLayout FVM_TextLayout;
    typedef enum FVM_TextAlignment
    {
        FVM_TEXT_ALIGN_LEFT,
        FVM_TEXT_ALIGN_CENTER,
        FVM_TEXT_ALIGN_RIGHT,
        FVM_TEXT_ALIGN_START
    } FVM_TextAlignment;
    typedef struct FVM_TextLayoutOptions
    {
        double max_width, line_height;
        FVM_TextDirection direction;
        FVM_TextAlignment alignment;
        const char *language;
    } FVM_TextLayoutOptions;
    typedef struct FVM_TextLayoutLine
    {
        size_t byte_start, byte_end, byte_next;
        size_t glyph_start, glyph_count;
        double x, baseline, advance;
        FVM_TextDirection direction;
    } FVM_TextLayoutLine;
    typedef struct FVM_TextLayoutGlyph
    {
        FVM_ShapedGlyph shaped;
        double x, y;
        size_t line;
        unsigned char bidi_level;
    } FVM_TextLayoutGlyph;
    typedef struct FVM_TextLayoutMetrics
    {
        double advance_width, height;
        FVM_PathBounds ink;
        size_t line_count, glyph_count;
    } FVM_TextLayoutMetrics;
    /* Options NULL selects unlimited width, natural line height, AUTO/START.
     * Owns glyph outlines and metadata: font, input and options may then be freed.
     * Limits: 4096 Unicode scalars / 16384 bytes, 1,000,000 trial-shaped scalars;
     * complexity overflow, tab stops and discretionary soft hyphens -> UNSUPPORTED.
     * CRLF is one separator; CR/LF/VT/FF/NEL/LS/PS create hard lines, including empty
     * trailing lines. Clusters and line byte ranges refer to the original UTF-8.
     * On failure, *out is NULL. No fonts profile -> UNSUPPORTED. */
    FVM_API FVM_Result fvm_text_layout_create(FVM_Font *, const char *, size_t,
                                              const FVM_TextLayoutOptions *, FVM_TextLayout **out);
    FVM_API void fvm_text_layout_destroy(FVM_TextLayout *);
    FVM_API FVM_Result fvm_text_layout_metrics(const FVM_TextLayout *, FVM_TextLayoutMetrics *);
    FVM_API FVM_Result fvm_text_layout_line(const FVM_TextLayout *, size_t, FVM_TextLayoutLine *);
    FVM_API FVM_Result fvm_text_layout_glyph(const FVM_TextLayout *, size_t, FVM_TextLayoutGlyph *);
    /* Return independent editable paths. Glyph paths include final positioning.
     * To select a word, use original byte clusters and append its glyph paths. */
    FVM_API FVM_Result fvm_text_layout_path(const FVM_TextLayout *, FVM_Path **out);
    FVM_API FVM_Result fvm_text_layout_glyph_path(const FVM_TextLayout *, size_t, FVM_Path **out);
#ifdef __cplusplus
}
#endif
#endif
