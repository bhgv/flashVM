/* OS: Windows. Open real shared SAPI dictation only after explicit host authorization. */
#include "../windows.hpp" /* Import microphone/grammar ownership and cleanup. */
#ifdef _WIN32 /* Keep native dictation off other toolchains. */
#include <new> /* Allocate unmanaged state without exceptions. */
#include <string.h> /* Clear all partial COM ownership fields. */
/* Activate a general-dictation grammar using the configured system recognizer. */
FVM_Result fvm_windows_stt_open(void *unused, void **out) /* Activation may open the configured microphone. */
{ /* Begin explicit native recognition. */
    (void)unused; /* The installed Windows recognizer selects its configured language/input. */
    *out = 0; /* Publish no partial recognition state. */
    FVM_WindowsStt *state = new (std::nothrow) FVM_WindowsStt; /* Allocate native ownership outside the GC. */
    if (!state) /* Do not activate capture after allocation failure. */
        return FVM_OUT_OF_MEMORY; /* Do not activate capture after allocation failure. */
    memset(state, 0, sizeof(*state)); /* Make partial-failure cleanup deterministic. */
    HRESULT result = CoInitializeEx(0, COINIT_APARTMENTTHREADED); /* Initialize the real thread-affine COM apartment. */
    if (FAILED(result)) /* Reject incompatible host apartment state. */
    { /* Release only the unpublished wrapper. */
        delete state; /* No successful COM initialization exists to balance. */
        return FVM_UNSUPPORTED; /* Require the host to supply a suitable native thread. */
    } /* Finish COM failure handling. */
    state->com = true; /* Own one successful apartment reference. */
    result = CoCreateInstance( /* Connect to the actual configured shared recognition service. */
        CLSID_SpSharedRecognizer, 0, CLSCTX_ALL, IID_ISpRecognizer, /* Connect to the actual configured shared recognition service. */
        (void **)&state->recognizer); /* Connect to the actual configured shared recognition service. */
    if (SUCCEEDED(result)) /* Create an isolated event queue for this session. */
        result = state->recognizer->CreateRecoContext( /* Create an isolated event queue for this session. */
            &state->context); /* Create an isolated event queue for this session. */
    if (SUCCEEDED(result)) /* Establish a real native notification source for queued speech events. */
        result = /* Establish a real native notification source for queued speech events. */
            state->context /* Establish a real native notification source for queued speech events. */
                ->SetNotifyWin32Event(); /* Establish a real native notification source for queued speech events. */
    if (SUCCEEDED(result)) /* Request only final transcript events. */
        result = state->context->SetInterest(SPFEI(SPEI_RECOGNITION), /* Request only final transcript events. */
                                             SPFEI(SPEI_RECOGNITION)); /* Request only final transcript events. */
    if (SUCCEEDED(result)) /* Own one general-dictation grammar. */
        result = state->context->CreateGrammar(0, &state->grammar); /* Own one general-dictation grammar. */
    if (SUCCEEDED(result)) /* Load the actual engine's default dictation topic. */
        result = state->grammar->LoadDictation(0, SPLO_STATIC); /* Load the actual engine's default dictation topic. */
    if (SUCCEEDED(result)) /* Explicitly activate microphone recognition. */
        result = state->grammar->SetDictationState(SPRS_ACTIVE); /* Explicitly activate microphone recognition. */
    if (FAILED(result)) /* Preserve unavailable-engine, permission and capture failures. */
    { /* Release every successfully initialized sibling resource. */
        fvm_windows_stt_close(state); /* Deactivate partial grammar state and balance COM. */
        return FVM_ERROR; /* Do not advertise a running recognizer after failure. */
    } /* Finish recognition-open failure handling. */
    *out = state; /* Transfer real active recognizer ownership to the caller. */
    return FVM_OK; /* Recognition continues until close; poll itself does not open capture. */
} /* Finish native dictation opening. */
#endif /* Finish Windows-only implementation. */
