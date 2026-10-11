#ifndef FVM_TEXT_STATE_HPP
#define FVM_TEXT_STATE_HPP
#include "../core/runtime.hpp"
/* Native text state is copied into the graph, never serialized as pointers.
   Layout caches are rebuilt from the embedded font faces in the receiver. */
struct FVM_FontSource
{
    fvm_detail::vector<unsigned char> bytes;
    long index;
};
struct FVM_TextStyleState
{
    fvm_detail::string font;
    double size, leading;
    unsigned long color, alignment;
    FVM_FontSource source;
    FVM_TextStyleState() : size(12), leading(2), color(0), alignment(0)
    {
        source.index = 0;
    }
};
struct FVM_TextFieldState
{
    FVM_TextStyleState current, future;
    fvm_detail::string text;
    double width, height;
    bool multiline, wrap;
    FVM_TextFieldState() : width(100), height(100), multiline(false), wrap(false)
    {
    }
};
size_t fvm_textfield_snapshot_size(FVM_Object *);
FVM_Result fvm_textfield_snapshot(FVM_Object *, FVM_TextFieldState &);
FVM_Result fvm_textfield_restore(FVM_Context *, FVM_Object *, const FVM_TextFieldState &);
const char *fvm_text_callback_selector(FVM_NativeFunction, void *);
void *fvm_text_callback_userdata(unsigned long, const char *);
#endif
