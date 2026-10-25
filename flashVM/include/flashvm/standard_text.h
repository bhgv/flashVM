/* FlashVM: Public embedding API declarations: standard text.
 * File: include/flashvm/standard_text.h.
 */

#ifndef FLASHVM_STANDARD_TEXT_H
/* Define FLASHVM STANDARD TEXT H constant or compilation switch. */
#define FLASHVM_STANDARD_TEXT_H
#include "flashvm/value.h"
#include "flashvm/text_layout.h"
#define FVM_TEXT_FONT_REGULAR 0U                                           /* Explicit unstyled family slot. */
#define FVM_TEXT_FONT_BOLD 1U                                              /* Family slot for a supplied bold face. */
#define FVM_TEXT_FONT_ITALIC 2U                                            /* Family slot for a supplied italic face. */
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


/* Copied field-local viewport observation; no borrowed pointers or managed roots. */
typedef struct FVM_TextFieldScrollInfo
{
    size_t struct_size;             /* Caller sets exact sizeof this structure. */
    unsigned long scroll;           /* Effective one-based top visual line. */
    unsigned long hscroll;          /* Effective zero-based horizontal pixel offset. */
    unsigned long maxscroll;        /* Largest admitted top line with this viewport. */
    unsigned long maxhscroll;       /* Ceiling of horizontal logical overflow in pixels. */
    unsigned long bottom_scroll;    /* Last fully fitting line, with at least the top line. */
    size_t line_count;              /* Visual layout lines, including empty hard lines. */
    double line_height;             /* Uniform baseline step from the current font and leading. */
    double offset_x, offset_y;      /* Scroll translation already applied by textfield_path. */
    double clip_x, clip_y;          /* Field-local inner viewport origin, after the gutter. */
    double clip_width, clip_height; /* Nonnegative inner viewport extents for host clipping. */
} FVM_TextFieldScrollInfo;


/* Owner-thread observation of host-created uniform TextField scroll geometry.
     * Requires matching context/object and exact struct_size. May build/cache layout;
     * no script/callback/device operation, no managed pointer retained. Cache failures
     * propagate (including NO_FONTS UNSUPPORTED and allocation OUT_OF_MEMORY); invalid
     * descriptors INVALID_ARGUMENT; excessive geometry UNSUPPORTED. Output unchanged
     * on failure. Keep object reachable; serialize all VM/field mutations and GC.
     * Successful observation clamps stored offsets to current geometry. Fully fitting
     * line policy keeps at least one top line even for a zero-height viewport. Native
     * contours remain unclipped; host applies returned rectangle and _x/_y separately.
     * This bounded uniform-style policy is not full Flash mixed-font/HTML/event parity. */
FVM_API FVM_Result fvm_textfield_scroll_info(
    FVM_Context *, FVM_Object *, FVM_TextFieldScrollInfo *out);
/* Independent editable contours, including the text field's 2-pixel gutter.
     * These are field-local: the host applies _x/_y exactly once when placing them.
     * Current effective scroll translation is included for glyphs and underline.
     * These are not clipped to the field rectangle; use scroll_info for host clipping.
     * Failed export preserves scroll offsets and clears out. */
/* Implement textfield path for this subsystem. */


/* Includes uniform underline when enabled; use nonzero fill for the combined contours. */
FVM_API FVM_Result fvm_textfield_path(FVM_Context *, FVM_Object *, FVM_Path **out);
#ifdef __cplusplus
}
#endif
#endif
