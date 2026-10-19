/* FlashVM: StartSound and SOUNDINFO wire representation limits. */
#ifndef FVM_SWF_SOUND_EVENT_CONSTANTS_H
#define FVM_SWF_SOUND_EVENT_CONSTANTS_H /* Guard scoped event parsing constants. */
#include "flashvm/swf_sound.h"
#define FVM_SWF_SOUND_EVENT_TAG 15U /* SWF1 StartSound root control tag. */
#define FVM_SWF_SOUND_EVENT_RESERVED 192U /* SOUNDINFO's two reserved high bits must be zero. */
#define FVM_SWF_SOUND_EVENT_MIN_BYTES 3U /* SoundId UI16 followed by SOUNDINFO flags. */
#define FVM_SWF_SOUND_ENVELOPE_BYTES 8U /* Pos44 UI32 and two UI16 volume levels. */
#define FVM_SWF_SOUND_ENVELOPE_LEVEL_MAX FVM_SWF_SOUND_ENVELOPE_UNITY /* Unity volume in the SWF fixed-point envelope. */
#define FVM_SWF_SOUND_EVENT_DEFAULT_PLAYS 1U /* Absent loop count plays once; explicit zero plays zero times. */
/* Native per-frame canonical-index allocation bound, separate from managed delivery tables. */
#define FVM_SWF_SOUND_MAX_FRAME_ORDINALS 65536UL
#endif
