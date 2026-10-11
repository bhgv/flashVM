#ifndef FLASHVM_FONT_H
#define FLASHVM_FONT_H
#include "flashvm/path.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct FVM_Font FVM_Font;
    typedef struct FVM_TextRun FVM_TextRun;
    typedef enum FVM_TextDirection
    {
        FVM_TEXT_AUTO,
        FVM_TEXT_LTR,
        FVM_TEXT_RTL
    } FVM_TextDirection;
    typedef struct FVM_FontMetrics
    {
        double ascent, descent, line_height;
        unsigned int glyph_count;
    } FVM_FontMetrics;
    typedef struct FVM_ShapedGlyph
    {
        unsigned int glyph, cluster;
        double x_advance, y_advance, x_offset, y_offset;
    } FVM_ShapedGlyph;
    /* Owns a copy of TTF/OpenType bytes, face index and an unhinted pixel size.
     * Each handle owns a FreeType library/face; do not use a handle concurrently.
     * Embedded bitmap/SVG/color glyph rendering is outside the outline facade. */
    FVM_API FVM_Result fvm_font_read(const void *bytes, size_t size, long face_index, double pixels,
                                     FVM_Font **out);
    FVM_API void fvm_font_destroy(FVM_Font *font);
    FVM_API FVM_Result fvm_font_metrics(const FVM_Font *font, FVM_FontMetrics *out);
    FVM_API unsigned int fvm_font_char(const FVM_Font *font, unsigned int unicode);
    /* Copies an unhinted glyph outline into editable, font-independent vectors.
     * Coordinates are pixels, x right/y down; advance is separate from ink bounds. */
    FVM_API FVM_Result fvm_font_glyph(FVM_Font *font, unsigned int glyph, FVM_Path **out,
                                      double *x_advance, double *y_advance);
    /* Shape one homogeneous script/direction run of valid UTF-8. AUTO guesses run
     * properties; it does not perform paragraph bidi or line layout. Clusters are
     * byte offsets into the input. Language may be NULL. Result survives font/input. */
    FVM_API FVM_Result fvm_font_shape(FVM_Font *font, const char *utf8, size_t size,
                                      FVM_TextDirection direction, const char *language,
                                      FVM_TextRun **out);
    FVM_API size_t fvm_text_run_count(const FVM_TextRun *run);
    FVM_API FVM_Result fvm_text_run_get(const FVM_TextRun *run, size_t index, FVM_ShapedGlyph *out);
    FVM_API void fvm_text_run_destroy(FVM_TextRun *run);
    /* Export placed contours; advance is logical run length, bounds from path are
     * geometric ink bounds. Pass the same font/size used to shape the run. */
    FVM_API FVM_Result fvm_text_run_path(FVM_Font *font, const FVM_TextRun *run, FVM_Path **out,
                                         double *x_advance, double *y_advance);
    /* libunibreak classifications, one byte per UTF-8 input byte; pass at least one
     * output array. Values are the native LINE/WORD/GRAPHEMEBREAK constants.
     * Finds allowed breaks; choosing lines by available width remains host policy. */
    FVM_API FVM_Result fvm_text_breaks(const char *utf8, size_t size, const char *language,
                                       char *line, char *word, char *grapheme);
    /* Resolve one UTF-32 paragraph/line with FriBidi. Outputs have count entries.
     * Direction is requested and resolved in-place; levels remain in logical order.
     * map[visual_index] is a logical index. Visual code points are reordered,
     * without mirroring or Arabic presentation-form substitution:
     * shape logical script/direction runs with HarfBuzz before arranging visually. */
    FVM_API FVM_Result fvm_text_bidi(const unsigned int *logical, size_t count,
                                     FVM_TextDirection *direction, unsigned int *visual,
                                     size_t *map, unsigned char *levels);
#ifdef __cplusplus
}
#endif
#endif
