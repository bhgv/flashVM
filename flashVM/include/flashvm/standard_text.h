/* FlashVM: Public embedding API declarations: standard text.
 * File: include/flashvm/standard_text.h.
 */

#ifndef FLASHVM_STANDARD_TEXT_H
/* Define FLASHVM STANDARD TEXT H constant or compilation switch. */
#define FLASHVM_STANDARD_TEXT_H
#include "flashvm/value.h"
#include "flashvm/text_layout.h"
#define FVM_TEXT_FONT_REGULAR 0U /* Explicit unstyled family slot. */
#define FVM_TEXT_FONT_BOLD 1U /* Family slot for a supplied bold face. */
#define FVM_TEXT_FONT_ITALIC 2U /* Family slot for a supplied italic face. */
#define FVM_TEXT_FONT_VARIANTS (FVM_TEXT_FONT_BOLD | FVM_TEXT_FONT_ITALIC) /* Bold-italic combines both. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Opt-in TextFormat and host-created TextField bridge. This is a tested subset,
     * not complete SWF display-list/TextField compatibility. Names must be unbound. */
    /* Implement standard text install for this subsystem. */
    FVM_API FVM_Result fvm_standard_text_install(FVM_Context *);
    /* Copies a validated TTF/OpenType face. Registration is per context; duplicate
     * names reject replacement. First font is the default. No system font lookup. */
    /* Register standard text font. */
    FVM_API FVM_Result fvm_standard_text_register_font(FVM_Context *, const char *name,
                                                       const void *bytes, size_t size,
                                                       long face_index);
    /* Copy a validated face into a named family/style slot; regular is flags zero.
     * Same owner-thread/GC restrictions and aggregate limits as register_font.
     * Duplicate family/variant or unknown bits -> INVALID_ARGUMENT, missing slot
     * during text formatting -> NOT_FOUND; no synthetic or system-font fallback.
     * Only the first regular registration sets the default family. Bytes are
     * borrowed during this call, then independently owned by the context.
     * Allocation failure -> OUT_OF_MEMORY; valid NO_FONTS input -> UNSUPPORTED. */
    FVM_API FVM_Result fvm_standard_text_register_font_variant(FVM_Context *, const char *family,
                                                       unsigned int variant, const void *bytes,
                                                       size_t size, long face_index);
    /* Creates a dynamic host text object, not a MovieClip depth/display-list node.
     * width/height include the 2-pixel gutter. Publish it with fvm_set_global. */
    /* Create textfield. */
    FVM_API FVM_Result fvm_textfield_create(FVM_Context *, const char *font_name, double width,
                                            double height, FVM_Object **out);
    /* Independent editable contours, including the text field's 2-pixel gutter.
     * These are field-local: the host applies _x/_y exactly once when placing them.
     * These are not clipped to the field rectangle. */
    /* Implement textfield path for this subsystem. */
    /* Includes uniform underline when enabled; use nonzero fill for the combined contours. */
    FVM_API FVM_Result fvm_textfield_path(FVM_Context *, FVM_Object *, FVM_Path **out);
#ifdef __cplusplus
}
#endif
#endif
