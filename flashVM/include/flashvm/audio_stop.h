/* FlashVM: context StopSounds bindings for prepared native mixers and host-owned queues. */
#ifndef FLASHVM_AUDIO_STOP_H
#define FLASHVM_AUDIO_STOP_H /* Guard the independent C89 audio-control ABI. */
#include "flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif
#define FVM_AUDIO_STOP_MAX_BINDINGS 64U /* Bound always-called controls independently of active PCM channels. */
/* Stop every pending/queued voice owned by userdata; callbacks must be idempotent. */
typedef FVM_Result (*FVM_AudioStopCallback)(void *userdata);
/* Context owns this registration until explicit detach or context destruction. */
typedef struct FVM_AudioStopBinding FVM_AudioStopBinding;
/* Copy callback/userdata; no callback on create. Clear *out on every failure. */
FVM_API FVM_Result fvm_audio_stop_binding_create(FVM_Context *context,
    FVM_AudioStopCallback callback, void *userdata, FVM_AudioStopBinding **out);
/* Detach without invoking stop, even after device failure. Null succeeds. */
FVM_API FVM_Result fvm_audio_stop_binding_close(FVM_AudioStopBinding *binding);
#ifdef __cplusplus
}
#endif
#endif
