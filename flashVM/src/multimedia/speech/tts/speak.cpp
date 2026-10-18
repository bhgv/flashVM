/* OS: provider-selected. Enqueue explicit UTF-8 speech through a real native backend. */
#include "../session.hpp" /* Import provider callbacks and text limits. */
#include "flashvm/utf8.h" /* Reuse strict Unicode-scalar validation for every provider. */
/* Validate string size before handing text to the native synthesizer. */
FVM_Result fvm_tts_speak(FVM_TtsSession *session, /* Borrow NUL-terminated UTF-8 only for this call. */
                         const char *text) /* Borrow NUL-terminated UTF-8 only for this call. */
{ /* Begin text submission. */
    if (!session || !text || !*text) /* Reject empty speech and absent ownership. */
        return FVM_INVALID_ARGUMENT; /* Reject empty speech and absent ownership. */
    size_t length = 0; /* Bound scanning by the documented payload limit. */
    while (length <= FVM_SPEECH_TEXT_LIMIT && text[length]) /* Require a terminator within the supported bound. */
        ++length; /* Require a terminator within the supported bound. */
    if (length > FVM_SPEECH_TEXT_LIMIT) /* Reject oversized requests before native allocation. */
        return FVM_INVALID_ARGUMENT; /* Reject oversized requests before native allocation. */
    if (fvm_utf8_validate(text, length, 0) != FVM_UTF8_OK) /* Reject malformed Unicode before any system call. */
        return FVM_INVALID_ARGUMENT; /* Reject malformed Unicode before any system call. */
    return session->backend.speak(session->native, text); /* Delegate actual synthesis/playback and its errors. */
} /* Finish explicit text submission. */
