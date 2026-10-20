/* Windows SAPI: open real dictation only after explicit host authorization.
 * Session interfaces and the successful COM reference belong to this thread. */
#include "../windows.hpp" /* Private native ownership and matching close function. */
#ifdef _WIN32
#include "../windows_constants.h" /* Strict C++98 equivalent of the SAPI event mask. */
#include <new>                    /* Nonthrowing allocation outside the VM collector. */
#include <string.h>               /* Deterministic partial-state initialization. */

/* Activate general dictation using the system recognizer and microphone.
 * Publish *out only after every COM/grammar stage succeeds; close rolls back
 * all interfaces and the apartment reference on any later failure. */
FVM_Result fvm_windows_stt_open(void *unused, void **out)
{
    (void)unused; /* Language/input selection belongs to the configured recognizer. */
    *out = 0;     /* Never expose an incompletely initialized native session. */
    FVM_WindowsStt *state = new (std::nothrow) FVM_WindowsStt;
    if (!state)
        return FVM_OUT_OF_MEMORY;
    memset(state, 0, sizeof(*state)); /* close can safely inspect every partial owner. */

    /* This explicit host call, not poll, initializes thread-affine COM access. */
    HRESULT result = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(result))
    {
        delete state; /* No successful initialization exists to balance. */
        return FVM_UNSUPPORTED;
    }
    state->com = true; /* Includes S_FALSE: every successful call needs a matching close. */

    /* Acquire the configured shared recognizer, then an isolated session queue. */
    result = CoCreateInstance(CLSID_SpSharedRecognizer, 0, CLSCTX_ALL,
                              IID_ISpRecognizer, (void **)&state->recognizer);
    if (SUCCEEDED(result))
        result = state->recognizer->CreateRecoContext(&state->context);
    if (SUCCEEDED(result))
        result = state->context->SetNotifyWin32Event();

    /* Request final transcripts plus the SDK's two required flag-check bits.
     * Cast-before-shift preserves Windows LLP64 without expanding ull literals. */
    if (SUCCEEDED(result))
        result = state->context->SetInterest(FVM_WINDOWS_SAPI_FINAL_EVENTS,
                                             FVM_WINDOWS_SAPI_FINAL_EVENTS);

    /* Own a general-dictation grammar and activate its actual microphone route. */
    if (SUCCEEDED(result))
        result = state->context->CreateGrammar(0, &state->grammar);
    if (SUCCEEDED(result))
        result = state->grammar->LoadDictation(0, SPLO_STATIC);
    if (SUCCEEDED(result))
        result = state->grammar->SetDictationState(SPRS_ACTIVE);
    if (FAILED(result))
    {
        fvm_windows_stt_close(state); /* Release siblings, deactivate and balance COM. */
        return FVM_ERROR;
    }

    *out = state; /* Transfer the complete active session to the caller until close. */
    return FVM_OK;
}
#endif
