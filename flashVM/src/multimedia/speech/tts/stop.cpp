/* OS: provider-selected. Cancel the selected provider's current/pending speech. */
#include "../session.hpp" /* Access the required native stop callback. */


/* Stop does not close the provider or affect other sessions. */
FVM_Result fvm_tts_stop(FVM_TtsSession *session) /* Keep provider ownership alive for later speak calls. */
{                                                /* Begin cancellation. */
    if (!session)                                /* Require a live output session. */
    {                                            /* Begin conditional branch: Require a live output session. */
        return FVM_INVALID_ARGUMENT;             /* Require a live output session. */
    } /* End conditional branch: Require a live output session. */
    return session->backend.stop(session->native); /* Invoke actual provider cancellation. */
} /* Finish speech cancellation. */
