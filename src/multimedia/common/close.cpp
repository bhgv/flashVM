/* OS: callback-selected. Release one host-owned stream exactly once. */
#include "session.hpp" /* Import decoder ownership and callbacks. */
/* Close tolerates null; callers must discard all copies of a non-null handle. */
void fvm_media_close(FVM_MediaSession *session) /* End a media session's lifetime. */
{ /* Begin cleanup. */
    if (!session) /* Make ordinary partial-initialization cleanup convenient. */
        return; /* Make ordinary partial-initialization cleanup convenient. */
    session->backend.close(session->stream); /* Release the actual library or device handle first. */
    delete session; /* Release the unmanaged wrapper last. */
} /* Finish cleanup. */
