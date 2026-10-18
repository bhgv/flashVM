/* OS: provider-selected. Query/copy one recognized transcript without a hidden microphone open. */
#include "../session.hpp" /* Import native recognition ownership. */
#include "flashvm/utf8.h" /* Validate provider Unicode before publishing a copied transcript. */
#include <string.h> /* Validate copied NUL termination without reading outside capacity. */
/* Zero capacity queries the next transcript length without consuming it. */
FVM_Result fvm_stt_poll(FVM_SttSession *session, char *text, size_t capacity, size_t *required, /* The caller owns its destination buffer. */
                        int *final) /* The caller owns its destination buffer. */
{ /* Begin nonblocking transcript polling. */
    if (required) /* Clear length even on invalid session arguments. */
        *required = 0; /* Clear length even on invalid session arguments. */
    if (final) /* Clear final/partial state before callback invocation. */
        *final = 0; /* Clear final/partial state before callback invocation. */
    if (text && capacity) /* Do not expose a previous transcript on failure. */
        text[0] = 0; /* Do not expose a previous transcript on failure. */
    if (!session || !required || !final || (capacity && !text)) /* Validate ownership and query/copy shape. */
        return FVM_INVALID_ARGUMENT; /* Validate ownership and query/copy shape. */
    FVM_Result result = session->backend.poll(session->native, text, capacity, required, /* Query the actual recognizer's retained transcript. */
                                              final); /* Query the actual recognizer's retained transcript. */
    if (result == FVM_OK && /* Continue the same native operation with the listed typed arguments. */
        (!*required || *required > FVM_SPEECH_TEXT_LIMIT + 1 || (*final != 0 && *final != 1) || /* Reject malformed successful provider output without reading past the buffer. */
         (capacity && (*required > capacity || text[*required - 1] != 0 || memchr(text, 0, *required - 1) || /* Reject malformed successful provider output without reading past the buffer. */
                       fvm_utf8_validate(text, *required - 1, 0) != FVM_UTF8_OK)))) /* Reject malformed successful provider output without reading past the buffer. */
        result = FVM_ERROR; /* Reject malformed successful provider output without reading past the buffer. */
    if (result != FVM_OK && text && capacity) /* Preserve a safe empty destination on failed copy. */
        text[0] = 0; /* Preserve a safe empty destination on failed copy. */
    return result; /* Preserve NOT_FOUND, insufficient-buffer INVALID_ARGUMENT and native errors. */
} /* Finish transcript polling. */
