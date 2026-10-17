/* OS: all supported targets. Private, unmanaged media-session ownership. */
#ifndef FVM_MEDIA_SESSION_HPP /* Protect the private declarations. */
#define FVM_MEDIA_SESSION_HPP /* Define the include guard. */
#include "flashvm/multimedia.h" /* Share the C89 bridge contract. */
struct FVM_MediaSession /* Keep C++ allocation outside the VM collector. */
{ /* Begin the host-owned stream wrapper. */
    FVM_MediaBackend backend; /* Copy callbacks so a temporary table may go out of scope. */
    void *stream; /* Own the backend-specific stream until close. */
    int kind; /* Validate returned buffers against the originally requested stream. */
}; /* Finish the private session layout. */
/* Validate a request, call its backend and publish only a fully opened session. */
FVM_Result fvm_media_open_internal(const FVM_MediaBackend *, const FVM_MediaRequest *, /* OS: callback-selected. */
                                   FVM_MediaSession **); /* OS: callback-selected. */
/* Check buffer geometry and overflow without retaining or dereferencing the buffer. */
bool fvm_media_frame_valid(const FVM_MediaFrame *, int); /* OS: portable validation. */
#endif /* Finish the include guard. */
