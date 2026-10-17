/* OS: callback-selected. Decode one buffer without changing VM execution state. */
#include "session.hpp" /* Import the copied decoder table. */
#include <string.h> /* Clear borrowed views on every unsuccessful read. */
/* Read and validate a backend frame; its storage remains backend-owned. */
FVM_Result fvm_media_read(FVM_MediaSession *session, FVM_MediaFrame *out) /* Expose one synchronous decode step. */
{ /* Begin frame retrieval. */
    if (!out) /* Require a writable frame view. */
        return FVM_INVALID_ARGUMENT; /* Require a writable frame view. */
    memset(out, 0, sizeof(*out)); /* Clear metadata even for an invalid session. */
    if (!session) /* Reject a missing stream wrapper. */
        return FVM_INVALID_ARGUMENT; /* Reject a missing stream wrapper. */
    FVM_Result result = session->backend.read(session->stream, out); /* Call the real decoder or host bridge. */
    if (result == FVM_OK && !fvm_media_frame_valid(out, session->kind)) /* Reject malformed successful callback output. */
        result = FVM_ERROR; /* Reject malformed successful callback output. */
    if (result != FVM_OK) /* Do not leak partially populated views. */
        memset(out, 0, sizeof(*out)); /* Do not leak partially populated views. */
    return result; /* Preserve EOF, unsupported and decoder error results. */
} /* Finish frame retrieval. */
