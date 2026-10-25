/* FlashVM: Public embedding API declarations: font.
 * File: include/flashvm/font.h.
 */

#ifndef FLASHVM_FONT_H
/* Define FLASHVM FONT H constant or compilation switch. */
#define FLASHVM_FONT_H
#include "flashvm/path.h"
#define FVM_FONT_SHAPE_NO_KERNING 1U                                                            /* Disable the OpenType kern feature for the entire run. */
#define FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES 2U                                                 /* Disable OpenType liga/clig, preserving required ligatures. */
#define FVM_FONT_SHAPE_FLAGS (FVM_FONT_SHAPE_NO_KERNING | FVM_FONT_SHAPE_NO_OPTIONAL_LIGATURES) /* Admitted feature mask. */
#ifdef __cplusplus
extern "C"
{
#endif
typedef struct FVM_Font FVM_Font;
typedef struct FVM_TextRun FVM_TextRun;


/* Define FVM_TextDirection enum and its representation. */
typedef enum FVM_TextDirection
{
    FVM_TEXT_AUTO /* Infer the base text direction. */,
    FVM_TEXT_LTR /* Use left-to-right base text direction. */,
    FVM_TEXT_RTL /* Use right-to-left base text direction. */
} FVM_TextDirection;


/* Define FVM_FontMetrics struct and its representation. */
typedef struct FVM_FontMetrics
{
    double ascent /* Distance from baseline to the font ascender. */,
        descent /* Distance from baseline to the font descender. */,
        line_height;          /* Recommended or configured distance between baselines. */
    unsigned int glyph_count; /* Number of glyph entries. */
} FVM_FontMetrics;


/* Font-defined underline geometry in pixel coordinates, positive y downward. */
typedef struct FVM_FontUnderlineMetrics
{
    double center_y;  /* Bar center relative to the text baseline. */
    double thickness; /* Positive full vertical bar thickness. */
} FVM_FontUnderlineMetrics;


/* Define FVM_ShapedGlyph struct and its representation. */
typedef struct FVM_ShapedGlyph
{
    unsigned int glyph /* Font glyph identifier. */,
        cluster; /* Input byte offset associated with this shaped glyph. */
    double x_advance /* Horizontal pen advance after this glyph. */,
        y_advance /* Vertical pen advance after this glyph. */,
        x_offset /* Horizontal glyph placement offset. */,
        y_offset; /* Vertical glyph placement offset. */
} FVM_ShapedGlyph;

/* Owns a copy of TTF/OpenType bytes, face index and an unhinted pixel size.
     * Each handle owns a FreeType library/face; do not use a handle concurrently.
     * Embedded bitmap/SVG/color glyph rendering is outside the outline facade. */


/* Read font. */
FVM_API FVM_Result fvm_font_read(const void *bytes, size_t size, long face_index, double pixels,
                                 FVM_Font **out);


/* Destroy font. */
FVM_API void fvm_font_destroy(FVM_Font *font);


/* Read ascender, descender, line height and glyph count of this font face. */
FVM_API FVM_Result fvm_font_metrics(const FVM_Font *font, FVM_FontMetrics *out);
/* Map a Unicode code point to a font glyph identifier. */


/* Read the positive OS/2 xAvgCharWidth scaled to unhinted pixels (26.6).
     * Borrow font on its owner thread; no allocations, VM calls or GC roots.
     * out is required and reset to zero. Missing/nonpositive/scalable metrics
     * return UNSUPPORTED; NULL font is INVALID_ARGUMENT in enabled builds.
     * NO_FONTS returns UNSUPPORTED with a valid out. No inferred fallback. */
FVM_API FVM_Result fvm_font_average_width(const FVM_Font *font, double *out);


/* Read scaled unhinted underline metrics from the borrowed scalable face.
     * Clears out on failure; NULL output/font is INVALID_ARGUMENT with fonts enabled.
     * Missing/nonpositive metrics -> UNSUPPORTED, without inventing a fallback.
     * NO_FONTS clears valid output and returns UNSUPPORTED. No allocation/VM/GC;
     * serialize access to a shared mutable font; copied scalar results are independent. */
FVM_API FVM_Result fvm_font_underline_metrics(const FVM_Font *, FVM_FontUnderlineMetrics *out);


FVM_API unsigned int fvm_font_char(const FVM_Font *font, unsigned int unicode);
/* Copies an unhinted glyph outline into editable, font-independent vectors.
     * Coordinates are pixels, x right/y down; advance is separate from ink bounds. */


/* Export an unhinted glyph outline as an editable path and report advances. */
FVM_API FVM_Result fvm_font_glyph(FVM_Font *font, unsigned int glyph, FVM_Path **out,
                                  double *x_advance, double *y_advance);
/* Shape one homogeneous script/direction run of valid UTF-8. AUTO guesses run
     * properties; it does not perform paragraph bidi or line layout. Clusters are
     * byte offsets into the input. Language may be NULL. Result survives font/input. */


/* Shape a UTF-8 script/direction run with HarfBuzz and retain glyph placements. */
FVM_API FVM_Result fvm_font_shape(FVM_Font *font, const char *utf8, size_t size,
                                  FVM_TextDirection direction, const char *language,
                                  FVM_TextRun **out);


/* Return the number of glyph entries in the shaped run. */
FVM_API size_t fvm_text_run_count(const FVM_TextRun *run);


/* Read one glyph placement and its source cluster from the shaped run. */
FVM_API FVM_Result fvm_text_run_get(const FVM_TextRun *run, size_t index, FVM_ShapedGlyph *out);


/* Like font_shape, with explicit feature flags (zero preserves legacy shaping).
     * NO_KERNING disables pair kerning without disabling ligatures/mark positioning.
     * NO_OPTIONAL_LIGATURES disables liga/clig without disabling rlig, marks or joining.
     * Combine flags with bitwise OR; legacy flags zero preserve all defaults.
     * Unknown flags are INVALID_ARGUMENT; out is cleared on failure. The font is
     * single-owner and borrowed only during this call; returned run is independently owned.
     * No VM/GC/callback or device access. NO_FONTS returns UNSUPPORTED for valid flags. */
FVM_API FVM_Result fvm_font_shape_configured(FVM_Font *, const char *, size_t,
                                             FVM_TextDirection, const char *, unsigned int flags,
                                             FVM_TextRun **out);


/* Release a shaped run and its retained glyph data. */
FVM_API void fvm_text_run_destroy(FVM_TextRun *run);
/* Export placed contours; advance is logical run length, bounds from path are
     * geometric ink bounds. Pass the same font/size used to shape the run. */


/* Export the positioned glyph outlines of a shaped run as one path. */
FVM_API FVM_Result fvm_text_run_path(FVM_Font *font, const FVM_TextRun *run, FVM_Path **out,
                                     double *x_advance, double *y_advance);
/* libunibreak classifications, one byte per UTF-8 input byte; pass at least one
     * output array. Values are the native LINE/WORD/GRAPHEMEBREAK constants.
     * Finds allowed breaks; choosing lines by available width remains host policy. */


/* Classify allowed line, word and grapheme breaks in UTF-8 input. */
FVM_API FVM_Result fvm_text_breaks(const char *utf8, size_t size, const char *language,
                                   char *line, char *word, char *grapheme);
/* Resolve one UTF-32 paragraph/line with FriBidi. Outputs have count entries.
     * Direction is requested and resolved in-place; levels remain in logical order.
     * map[visual_index] is a logical index. Visual code points are reordered,
     * without mirroring or Arabic presentation-form substitution:
     * shape logical script/direction runs with HarfBuzz before arranging visually.
     * At most 1,000,000 Unicode scalars; surrogate/non-Unicode values and invalid
     * direction representations return INVALID_ARGUMENT before changing any output.
     * All arrays are borrowed for the call; logical and visual may be the same array.
     * Other output arrays must be disjoint. Errors leave direction/outputs unchanged.
     * Empty input permits NULL arrays; AUTO resolves to LTR with fonts enabled.
     * NO_FONTS validates inputs then returns UNSUPPORTED without writing outputs.
     * No VM/GC state is accessed; independent caller buffers can run concurrently. */


/* Resolve bidi levels and the visual-to-logical mapping of a paragraph. */
FVM_API FVM_Result fvm_text_bidi(const unsigned int *logical, size_t count,
                                 FVM_TextDirection *direction, unsigned int *visual,
                                 size_t *map, unsigned char *levels);
#ifdef __cplusplus
}
#endif
#endif
