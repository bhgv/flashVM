#ifndef FLASHVM_STANDARD_TEXT_H
#define FLASHVM_STANDARD_TEXT_H
#include "flashvm/value.h"
#include "flashvm/text_layout.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Opt-in TextFormat and host-created TextField bridge. This is a tested subset,
     * not complete SWF display-list/TextField compatibility. Names must be unbound. */
    FVM_API FVM_Result fvm_standard_text_install(FVM_Context *);
    /* Copies a validated TTF/OpenType face. Registration is per context; duplicate
     * names reject replacement. First font is the default. No system font lookup. */
    FVM_API FVM_Result fvm_standard_text_register_font(FVM_Context *, const char *name,
                                                       const void *bytes, size_t size,
                                                       long face_index);
    /* Creates a dynamic host text object, not a MovieClip depth/display-list node.
     * width/height include the 2-pixel gutter. Publish it with fvm_set_global. */
    FVM_API FVM_Result fvm_textfield_create(FVM_Context *, const char *font_name, double width,
                                            double height, FVM_Object **out);
    /* Independent editable contours, including the text field's 2-pixel gutter.
     * These are not clipped to the field rectangle. */
    FVM_API FVM_Result fvm_textfield_path(FVM_Context *, FVM_Object *, FVM_Path **out);
#ifdef __cplusplus
}
#endif
#endif
