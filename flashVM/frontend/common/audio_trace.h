/* Optional bounded native PCM evidence: RAM during playback, files only after worker shutdown. */
#ifndef FVM_DESKTOP_AUDIO_TRACE_H
#define FVM_DESKTOP_AUDIO_TRACE_H
#include "flashvm/swf_sound_output.h"

/* Opaque host owner; contains no VM pointers and has no internal synchronization. */
typedef struct FVM_DesktopAudioTrace FVM_DesktopAudioTrace;
#ifdef __cplusplus
extern "C"
{
#endif


/* Open only when FVM_AUDIO_TRACE names an explicit output basename; allocation failure disables
 * capture. */
FVM_DesktopAudioTrace *fvm_desktop_trace_open(const FVM_SDLAudioQueueInfo *, size_t target, size_t resume);


/* Record one serialized operation, its relative wall time and native-frame position in bounded RAM.
 */
void fvm_desktop_trace_event(FVM_DesktopAudioTrace *, const char *operation, unsigned long value, FVM_Result);


/* Copy exactly one accepted queue block; do not change playback status when capture is full. */
void fvm_desktop_trace_pcm(FVM_DesktopAudioTrace *, const short *, size_t frames);


/* Write portable little-endian WAV and operation report after the worker has been joined, then
 * free. */
void fvm_desktop_trace_close(FVM_DesktopAudioTrace *);
#ifdef __cplusplus
}
#endif
#endif
