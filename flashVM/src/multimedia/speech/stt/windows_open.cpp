/* Windows SAPI: open actual dictation after explicit host authorization.
 * The successful COM apartment reference and all interfaces belong to this thread. */
#include "../windows.hpp"         /* Declare native owners and the matching rollback/close operation. */

#ifdef _WIN32                     /* Compile these SDK calls only for the Windows target. */
#include "../windows_constants.h" /* Use C++98-safe 64-bit SAPI event masks. */
#include <new>                    /* Allocate native session storage without throwing through the C ABI. */
#include <string.h>               /* Zero partial owners so rollback can inspect every field. */


/* Activate the configured system recognizer's general-dictation microphone route.
 * Parameters: unused is reserved borrowed backend data; out must point to a writable
 * session slot and receives ownership only after every COM/grammar stage succeeds.
 * Returns OK, OUT_OF_MEMORY, UNSUPPORTED for apartment initialization failure, or
 * ERROR after rolling back a later SAPI failure. Close on the same owning thread.
 * Call only after the host has authorized recognition; poll does not activate it. */
FVM_Result fvm_windows_stt_open(void *unused, void **out)      /* Open a thread-affine, explicitly owned native session. */
{                                                              /* Start with no published session, including all failure paths. */
    (void)unused;                                              /* Input/language selection belongs to the configured system recognizer. */
    *out = 0;                                                  /* Do not expose partial COM or grammar ownership. */
    FVM_WindowsStt *state = new (std::nothrow) FVM_WindowsStt; /* Allocate only native bookkeeping, outside the VM heap. */
    if (!state)                                                /* Stop before touching COM when bookkeeping allocation fails. */
    {                                                          /* Report the actual allocation failure without opening any device. */
        return FVM_OUT_OF_MEMORY;                              /* The output slot remains empty. */
    } /* Continue only with an owned bookkeeping object. */
    memset(state, 0, sizeof(*state));                             /* Make every not-yet-acquired interface safe for the close routine. */
    HRESULT result = CoInitializeEx(0, COINIT_APARTMENTTHREADED); /* Acquire this thread's matching COM apartment reference. */
    if (FAILED(result))                                           /* An incompatible/unavailable apartment cannot own a recognition session. */
    {                                                             /* Only the native bookkeeping object exists at this point. */
        delete state;                                             /* Do not balance a COM initialization that did not succeed. */
        return FVM_UNSUPPORTED;                                   /* Leave the session unpublished. */
    } /* S_FALSE is successful too and must still be balanced by close. */
    state->com = true;                                                         /* Tell rollback/close that this apartment reference is owned. */
    result = CoCreateInstance(CLSID_SpSharedRecognizer, 0, CLSCTX_ALL,         /* Acquire the configured shared recognizer. */
                              IID_ISpRecognizer, (void **)&state->recognizer); /* Store only a successfully acquired recognizer interface. */
    if (SUCCEEDED(result))                                                     /* Skip dependent operations after any preceding failure. */
    {                                                                          /* Create an isolated recognition context/notification queue for this session. */
        result = state->recognizer->CreateRecoContext(&state->context);        /* Store its owned COM interface for later rollback. */
    } /* Preserve the first failed HRESULT for the common cleanup path. */
    if (SUCCEEDED(result))                              /* Event notification requires the acquired recognition context. */
    {                                                   /* Use a real Win32 event instead of executing arbitrary callbacks during recognition. */
        result = state->context->SetNotifyWin32Event(); /* Configure the context's notification mechanism. */
    } /* Later polling can inspect notifications without reopening the microphone. */
    if (SUCCEEDED(result))                                                   /* Install interests only after notification setup succeeds. */
    {                                                                        /* Request completed recognition plus both flag-check bits required by SAPI. */
        result = state->context->SetInterest(FVM_WINDOWS_SAPI_FINAL_EVENTS,  /* Apply the complete event-interest mask. */
                                             FVM_WINDOWS_SAPI_FINAL_EVENTS); /* Use the same mask for queued event delivery. */
    } /* Interim recognition remains outside this final-transcript subset. */
    if (SUCCEEDED(result))                                          /* Grammar creation requires the live context and configured queue. */
    {                                                               /* Acquire the session's general-dictation grammar owner. */
        result = state->context->CreateGrammar(0, &state->grammar); /* Let close release this interface on any later failure. */
    } /* Keep ownership unpublished until both loading and activation succeed. */
    if (SUCCEEDED(result))                                      /* Do not dereference a grammar that was never acquired. */
    {                                                           /* Load the configured static dictation grammar. */
        result = state->grammar->LoadDictation(0, SPLO_STATIC); /* Use the recognizer's default topic/configuration. */
    } /* Retain the HRESULT for either activation or cleanup. */
    if (SUCCEEDED(result))                                       /* Activation is permitted only for a successfully loaded grammar. */
    {                                                            /* This is the explicit operation that starts the recognition route. */
        result = state->grammar->SetDictationState(SPRS_ACTIVE); /* Start actual system recognition after host authorization. */
    } /* An activation failure still owns interfaces that must be released. */
    if (FAILED(result))               /* Any failed recognizer/context/grammar stage uses one rollback path. */
    {                                 /* Release partial owners before returning an ordinary API error. */
        fvm_windows_stt_close(state); /* Deactivate/release interfaces and balance the successful COM reference. */
        return FVM_ERROR;             /* Leave the caller's output slot empty after rollback. */
    } /* Only the fully active native session reaches publication. */
    *out = state;  /* Transfer the completed session to the caller until its same-thread close. */
    return FVM_OK; /* The host may now poll final transcript notifications. */
} /* End explicit session activation; this operation does not run on a GC worker. */
#endif /* End the Windows-only implementation. */
