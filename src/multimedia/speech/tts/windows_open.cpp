/* OS: Windows. Create the actual SAPI voice on the session's calling thread. */
#include "../windows.hpp" /* Import native ownership and cleanup. */
#ifdef _WIN32 /* Exclude Windows SDK calls from other targets. */
#include <new> /* Allocate native state without exceptions. */
/* Initialize one STA apartment reference and one native voice. */
FVM_Result fvm_windows_tts_open(void *unused, /* The installed system voice supplies language/device defaults. */
                                void **out) /* The installed system voice supplies language/device defaults. */
{ /* Begin native output initialization. */
    (void)unused; /* No shared mutable configuration is retained. */
    *out = 0; /* Publish only fully initialized native state. */
    FVM_WindowsTts *state = new (std::nothrow) FVM_WindowsTts; /* Allocate unmanaged ownership. */
    if (!state) /* Avoid COM calls after wrapper failure. */
        return FVM_OUT_OF_MEMORY; /* Avoid COM calls after wrapper failure. */
    state->voice = 0; /* Establish partial-cleanup safety. */
    state->com = false; /* No apartment reference is owned yet. */
    HRESULT result = /* Initialize the real COM apartment on this thread. */
        CoInitializeEx(0, COINIT_APARTMENTTHREADED); /* Initialize the real COM apartment on this thread. */
    if (FAILED(result)) /* An incompatible existing apartment must be handled by the host. */
    { /* Release the unpublished wrapper. */
        delete state; /* Do not CoUninitialize a failed initialization. */
        return FVM_UNSUPPORTED; /* Report the incompatible/unavailable native runtime. */
    } /* Finish COM failure handling. */
    state->com = true; /* S_OK and S_FALSE both require a matching CoUninitialize. */
    result = CoCreateInstance(CLSID_SpVoice, 0, CLSCTX_ALL, IID_ISpVoice, /* Open the actual installed SAPI voice. */
                              (void **)&state->voice); /* Open the actual installed SAPI voice. */
    if (FAILED(result)) /* Handle missing voice engines without pretending to synthesize. */
    { /* Release the COM reference and wrapper. */
        fvm_windows_tts_close(state); /* Balance successful initialization. */
        return FVM_UNSUPPORTED; /* A target may have no usable installed voice. */
    } /* Finish voice-open failure handling. */
    *out = state; /* Transfer complete native voice ownership. */
    return FVM_OK; /* No text is spoken until an explicit speak call. */
} /* Finish native voice creation. */
#endif /* Finish Windows-only implementation. */
