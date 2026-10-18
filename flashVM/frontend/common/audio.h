/* Main-thread event-sound ownership for the optional desktop player. */
#ifndef FVM_DESKTOP_AUDIO_H
#define FVM_DESKTOP_AUDIO_H
#include "flashvm/swf_sound_output.h"
#define FVM_DESKTOP_AUDIO_RATE 48000U  /* Fixed native output clock, independent of movie FPS. */
#define FVM_DESKTOP_AUDIO_CHANNELS 2U  /* Stereo interleaved native S16. */
#define FVM_DESKTOP_AUDIO_VOICES 32U   /* Bounded overlapping event voices. */
#define FVM_DESKTOP_AUDIO_BLOCK 512U   /* At most one small preparation per pump. */
#define FVM_DESKTOP_AUDIO_TARGET 2048U /* Approximately 43 ms software queue at 48 kHz. */
#define FVM_DESKTOP_AUDIO_PUMPS 4U     /* Bound refill work per host-loop visit. */
#define FVM_DESKTOP_AUDIO_WAIT 5       /* Maximum event wait while audio needs regular refilling. */
typedef struct FVM_DesktopAudio
{
    FVM_SWFSoundMixer *mixer;   /* Owned independent native voice snapshots. */
    FVM_SWFSoundOutput *output; /* Owned pending PCM, borrowing mixer/device. */
    FVM_SDLAudioQueue *device;  /* Owned real playback endpoint, closed before SDL shutdown. */
    unsigned long events;       /* Successfully submitted real frame-entry controls. */
} FVM_DesktopAudio;
/* Open only for a file containing root event sound; disabled SDK errors remain explicit. */
FVM_Result fvm_desktop_audio_open(FVM_Context *, FVM_Object *, FVM_DesktopAudio *);
/* Fill bounded PCM and explicitly resume, including while waiting for a slow video frame. */
FVM_Result fvm_desktop_audio_tick(FVM_DesktopAudio *);
/* Stop/free all native audio dependencies before window close calls SDL_Quit. */
void fvm_desktop_audio_close(FVM_DesktopAudio *);
#endif
