/* OS: host-selected audio bridge. Context registrations contain no managed objects. */
#ifndef FVM_AUDIO_CHANNEL_HPP /* Guard the private channel layout and callback barrier. */
#define FVM_AUDIO_CHANNEL_HPP /* Declare this private header once. */
#include "../common/session.hpp" /* Reuse the public ABI and PCM frame validation. */
#include "../../core/native_call_guard.hpp" /* [GC:] Prevent collection while borrowed host state is active. */
struct FVM_AudioChannel /* Register one independently stoppable host output. */
{ /* Begin the context-owned registration. */
    FVM_Context *context; /* Borrow the owner until close or context destruction. */
    FVM_MediaOutput output; /* Copy the callback table; userdata remains host-owned. */
    bool active; /* Record successful submission until successful stop. */
}; /* Finish the non-managed channel layout. */
/* Accept VM-thread operations outside destruction, collection and callback mutation. */
inline bool fvm_audio_context_valid(FVM_Context *context) /* OS: portable lifecycle checks. */
{ /* Inspect lifecycle without entering host code. */
    return context && !context->destroying && !context->gc_collecting && /* Reject invalid context phases. */
           !context->gc_releasing && !context->audio_busy; /* Keep the registry stable through callbacks. */
} /* Finish context validation. */
class FVM_AudioCallbackGuard /* [GC:] Block collection and registry mutation during a host callback. */
{ /* Begin the noncopyable callback scope. */
    FVM_Context *context_; /* Borrow the context protected by this scope. */
    FVM_NativeCallGuard native_; /* [GC:] Retain the ordinary native collection barrier. */
    FVM_AudioCallbackGuard(const FVM_AudioCallbackGuard &); /* Prevent duplicate restoration. */
    FVM_AudioCallbackGuard &operator=(const FVM_AudioCallbackGuard &); /* Prevent transferring an active scope. */
  public: /* Expose only balanced scope entry and exit. */
    explicit FVM_AudioCallbackGuard(FVM_Context *context) : context_(context), native_(context) /* Enter the native barrier first. */
    { /* Begin callback registry protection. */
        context_->audio_busy = true; /* Reject create/write/stop/close and nested stop-all. */
    } /* Finish callback scope entry. */
    ~FVM_AudioCallbackGuard() /* Restore registry access after the host callback returns. */
    { /* Begin balanced scope cleanup. */
        context_->audio_busy = false; /* [GC:] The member barrier is released immediately after this destructor body. */
    } /* Finish callback scope cleanup. */
}; /* Finish the callback guard. */
/* Stop active outputs once and release registrations during context destruction. */
void fvm_audio_context_cleanup(FVM_Context *); /* OS: host callbacks; errors cannot escape void context destruction. */
#endif /* Finish the private header guard. */
