/* OS: Windows. Deactivate this grammar and release real microphone-recognition resources. */
#include "../windows.hpp" /* Import native dictation ownership. */
#ifdef _WIN32             /* Isolate COM/SAPI cleanup. */


/* Close complete and partial recognition state on the creating thread. */
void fvm_windows_stt_close(void *opaque)                           /* Closing this session does not stop other clients' shared grammars. */
{                                                                  /* Begin native recognition cleanup. */
    FVM_WindowsStt *state = static_cast<FVM_WindowsStt *>(opaque); /* Recover this session's resources. */
    if (state->grammar)                                            /* Deactivate only a successfully created grammar. */
    {                                                              /* Stop this session's dictation before releasing its input references. */
        state->grammar->SetDictationState(SPRS_INACTIVE);          /* Stop this client's active recognition. */
        state->grammar->Release();                                 /* Release the actual native grammar. */
    } /* Finish grammar cleanup. */
    if (state->context)            /* Release this session's event queue. */
    {                              /* Begin conditional branch: Release this session's event queue. */
        state->context->Release(); /* Release this session's event queue. */
    } /* End conditional branch: Release this session's event queue. */
    if (state->recognizer)            /* Release the shared recognition-service reference. */
    {                                 /* Begin conditional branch: Release the shared recognition-service reference. */
        state->recognizer->Release(); /* Release the shared recognition-service reference. */
    } /* End conditional branch: Release the shared recognition-service reference. */
    CoTaskMemFree(state->pending); /* Release any transcript retained after a length query. */
    if (state->com)                /* Balance this session's successful COM initialization. */
    {                              /* Begin conditional branch: Balance this session's successful COM initialization. */
        CoUninitialize();          /* Balance this session's successful COM initialization. */
    } /* End conditional branch: Balance this session's successful COM initialization. */
    delete state; /* Release unmanaged session ownership. */
} /* Finish native recognition cleanup. */
#endif /* Finish Windows-only implementation. */
