/* FlashVM: Built-in ActionScript objects: text state.
 * File: src/standard/text_state.hpp.
 */

#ifndef FVM_TEXT_STATE_HPP
/* Define FVM TEXT STATE HPP constant or compilation switch. */
#define FVM_TEXT_STATE_HPP
#include "../core/runtime.hpp"
/* Native text state is copied into the graph, never serialized as pointers.
   Layout caches are rebuilt from the embedded font faces in the receiver. */
/* Define FVM_FontSource struct and its representation. */
struct FVM_FontSource
{
    fvm_detail::vector<unsigned char>
        bytes;  /* Byte count or byte storage associated with this state. */
    long index; /* Current position within the associated sequence. */
};

/* Define FVM_TextStyleState struct and its representation. */
struct FVM_TextStyleState
{
    fvm_detail::string font; /* Font name, handle or source used by this text state. */
    double size /* Size of the associated payload or collection. */,
        leading; /* Extra spacing added between text lines. */
    unsigned long color /* Text color used for rendering. */,
        alignment;         /* Low alignment bits plus the explicit NO_KERNING transfer flag. */
    FVM_FontSource source; /* Source name associated with this entry. */
    /* Initialize Text Style State and its owned state. */
    FVM_TextStyleState() : size(12), leading(2), color(0), alignment(0)
    {
        source.index = 0;
    }
};

/* Define FVM_TextFieldState struct and its representation. */
struct FVM_TextFieldState
{
    FVM_TextStyleState current /* Current buffered source byte. */,
        future;              /* Lookahead bytes buffered by the parser. */
    fvm_detail::string text; /* Text content retained by this state. */
    double width /* Horizontal extent in the API coordinate system. */,
        height; /* Vertical extent in the API coordinate system. */
    bool multiline /* Whether multiple text lines are permitted. */,
        wrap; /* Whether line wrapping is enabled. */
    /* Initialize Text Field State and its owned state. */
    FVM_TextFieldState() : width(100), height(100), multiline(false), wrap(false)
    {
    }
};

/* Capture snapshot of textfield size. */
size_t fvm_textfield_snapshot_size(FVM_Object *);

/* Capture snapshot of textfield. */
FVM_Result fvm_textfield_snapshot(FVM_Object *, FVM_TextFieldState &);

/* Restore textfield. */
FVM_Result fvm_textfield_restore(FVM_Context *, FVM_Object *, const FVM_TextFieldState &);

/* Resolve the serialized selector for a native text callback. */
const char *fvm_text_callback_selector(FVM_NativeFunction, void *);

/* Restore text callback data from its serialized identifier and selector. */
void *fvm_text_callback_userdata(unsigned long, const char *);
/* [GC:] Expose the native text buffer as an implicit root of a reachable TextField. */
const char *fvm_textfield_string_root(FVM_Object *);
#endif
