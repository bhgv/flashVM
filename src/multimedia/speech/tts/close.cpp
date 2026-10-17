/* OS: provider-selected. Release one TTS session and its native resources. */
#include "../session.hpp" /* Access copied native cleanup callbacks. */
/* Close tolerates null; a non-null session must be closed exactly once. */
void fvm_tts_close(FVM_TtsSession *session) /* Do not race with other calls on this session. */
{ /* Begin speech cleanup. */
    if (!session) /* Support ordinary partial-initialization cleanup. */
        return; /* Support ordinary partial-initialization cleanup. */
    session->backend.close(session->native); /* Stop/release actual speech or microphone resources. */
    delete session; /* Release unmanaged session ownership last. */
} /* Finish speech cleanup. */
