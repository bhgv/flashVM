/* FlashVM: version-aware AS1/AS2 URL global callbacks installed with standard objects. */
#ifndef FVM_STANDARD_URL_ENCODING_HPP
#define FVM_STANDARD_URL_ENCODING_HPP /* Guard the private URL callback declarations. */
#include "flashvm/value.h"


/* Encode counted VM bytes, retaining only ASCII letters/digits; absent input returns undefined. */
FVM_Result fvm_builtin_escape(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Decode canonical %HH/plus input; malformed escapes or invalid decoded WTF-8 are unsupported. */
FVM_Result fvm_builtin_unescape(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Only these codecs may collect during rooted ToString coercion after graph restoration. */
inline bool fvm_url_collector_callback(FVM_NativeFunction callback)
{
    return callback == fvm_builtin_escape || callback == fvm_builtin_unescape;
}
#endif
