/* OS: backend-selected. Open an explicit camera source without hidden device access. */
#include "../common/session.hpp" /* Share common ownership and validation. */
/* Create a camera request and forward it to the selected real backend. */
FVM_Result fvm_camera_open(const FVM_MediaBackend *backend, const char *url, const char *format, /* Borrow input strings during open only. */
                           FVM_MediaSession **out) /* Borrow input strings during open only. */
{ /* Begin the camera adapter. */
    if (!format || !*format) /* Check this precondition before entering the following operation. */
    { /* Continue the same operation using the explicitly listed arguments. */
        if (out) /* Require an explicit device format. */
            *out = 0; /* Require an explicit device format. */
        return FVM_INVALID_ARGUMENT; /* Return the result produced by the following backend operation. */
    } /* Require an explicit device format. */
    FVM_MediaRequest request; /* Keep the immutable request on the calling thread's stack. */
    request.struct_size = sizeof(request); /* Identify the exact request layout. */
    request.kind = FVM_MEDIA_CAMERA; /* Select the required stream category. */
    request.url = url; /* Pass the explicit host filename or device identifier. */
    request.format = format; /* Select automatic file probing or an explicit camera input format. */
    return fvm_media_open_internal(backend, &request, out); /* Reuse safe stream publication and ownership. */
} /* Finish the camera open adapter. */
