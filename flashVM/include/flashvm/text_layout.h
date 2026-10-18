/* FlashVM: Public embedding API declarations: text layout.
 * File: include/flashvm/text_layout.h.
 */

#ifndef FLASHVM_TEXT_LAYOUT_H
/* Define FLASHVM TEXT LAYOUT H constant or compilation switch. */
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
    /* Define FVM_TextAlignment enum and its representation. */
    typedef enum FVM_TextAlignment
    {
        FVM_TEXT_ALIGN_LEFT /* Text alignment: left. */,
        FVM_TEXT_ALIGN_CENTER /* Text alignment: center. */,
        FVM_TEXT_ALIGN_RIGHT /* Text alignment: right. */,
        FVM_TEXT_ALIGN_START /* Text alignment: start. */
    } FVM_TextAlignment;
    /* Define FVM_TextLayoutOptions struct and its representation. */
    typedef struct FVM_TextLayoutOptions
    {
        double max_width /* Width constraint used to choose line breaks. */,
            line_height;             /* Recommended or configured distance between baselines. */
        FVM_TextDirection direction; /* Direction of text or a recognized gesture. */
        FVM_TextAlignment alignment; /* Requested horizontal text alignment. */
        const char *language;        /* Language tag used for shaping or line breaking. */
    } FVM_TextLayoutOptions;
    /* Define FVM_TextLayoutLine struct and its representation. */
    typedef struct FVM_TextLayoutLine
    {
        size_t byte_start /* First input byte offset of this span. */,
            byte_end /* Input byte offset past the end of this span. */,
            byte_next; /* Input byte offset where the next span starts. */
        size_t glyph_start /* Index of the first glyph in this span. */,
            glyph_count; /* Number of glyph entries. */
        double x /* Horizontal coordinate. */,
            baseline /* Vertical coordinate of the line baseline. */,
            advance;                 /* Logical pen advance. */
        FVM_TextDirection direction; /* Direction of text or a recognized gesture. */
    } FVM_TextLayoutLine;
    /* Define FVM_TextLayoutGlyph struct and its representation. */
    typedef struct FVM_TextLayoutGlyph
    {
        FVM_ShapedGlyph shaped;                   /* Shaped glyph run owned by the layout. */
        double x /* Horizontal coordinate. */, y; /* Vertical coordinate. */
        size_t line;              /* Source line number associated with this entry. */
        unsigned char bidi_level; /* Resolved bidirectional embedding level. */
    } FVM_TextLayoutGlyph;
    /* Define FVM_TextLayoutMetrics struct and its representation. */
    typedef struct FVM_TextLayoutMetrics
    {
        double advance_width /* Logical horizontal advance of the text. */,
            height;         /* Vertical extent in the API coordinate system. */
        FVM_PathBounds ink; /* Geometric bounds of the glyph outlines. */
        size_t line_count /* Number of laid-out lines. */,
            glyph_count; /* Number of glyph entries. */
    } FVM_TextLayoutMetrics;
    /* Options NULL selects unlimited width, natural line height, AUTO/START.
     * Owns glyph outlines and metadata: font, input and options may then be freed.
     * Limits: 4096 Unicode scalars / 16384 bytes, 1,000,000 trial-shaped scalars;
     * complexity overflow, tab stops and discretionary soft hyphens -> UNSUPPORTED.
     * CRLF is one separator; CR/LF/VT/FF/NEL/LS/PS create hard lines, including empty
     * trailing lines. Clusters and line byte ranges refer to the original UTF-8.
     * On failure, *out is NULL. No fonts profile -> UNSUPPORTED. */
    /* Create text layout. */
    FVM_API FVM_Result fvm_text_layout_create(FVM_Font *, const char *, size_t,
                                              const FVM_TextLayoutOptions *, FVM_TextLayout **out);
    /* Destroy text layout. */
    FVM_API void fvm_text_layout_destroy(FVM_TextLayout *);
    /* Implement text layout metrics for this subsystem. */
    FVM_API FVM_Result fvm_text_layout_metrics(const FVM_TextLayout *, FVM_TextLayoutMetrics *);
    /* Implement text layout line for this subsystem. */
    FVM_API FVM_Result fvm_text_layout_line(const FVM_TextLayout *, size_t, FVM_TextLayoutLine *);
    /* Implement text layout glyph for this subsystem. */
    FVM_API FVM_Result fvm_text_layout_glyph(const FVM_TextLayout *, size_t, FVM_TextLayoutGlyph *);
    /* Return independent editable paths. Glyph paths include final positioning.
     * To select a word, use original byte clusters and append its glyph paths. */
    /* Implement text layout path for this subsystem. */
    FVM_API FVM_Result fvm_text_layout_path(const FVM_TextLayout *, FVM_Path **out);
    /* Implement text layout glyph path for this subsystem. */
    FVM_API FVM_Result fvm_text_layout_glyph_path(const FVM_TextLayout *, size_t, FVM_Path **out);
#ifdef __cplusplus
}
#endif
#endif
