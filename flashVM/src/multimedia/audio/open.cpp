/* OS: backend-selected. Open an explicit audio source without hidden device access. */
#include "../common/session.hpp" /* Share common ownership and validation. */


/* Create a audio request and forward it to the selected real backend. */
FVM_Result fvm_audio_open(const FVM_MediaBackend *backend, const char *url, /* Borrow input strings during open only. */
                          FVM_MediaSession **out)                           /* Borrow input strings during open only. */
{                                                                           /* Begin the audio adapter. */
    FVM_MediaRequest request;                                               /* Keep the immutable request on the calling thread's stack. */
    request.struct_size = sizeof(request);                                  /* Identify the exact request layout. */
    request.kind = FVM_MEDIA_AUDIO;                                         /* Select the required stream category. */
    request.url = url;                                                      /* Pass the explicit host filename or device identifier. */
    request.format = 0;                                                     /* Select automatic file probing or an explicit camera input format. */
    return fvm_media_open_internal(backend, &request, out);                 /* Reuse safe stream publication and ownership. */
} /* Finish the audio open adapter. */
