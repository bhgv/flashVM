/* Wrap only owner-side voice preparation; retain the real production worker and SDL queue. */
#define fvm_swf_sound_voice_open_bounded delayed_voice_open
#include "../common/audio.c"
#undef fvm_swf_sound_voice_open_bounded
FVM_Result fvm_swf_sound_voice_open_bounded(FVM_Context *, size_t, size_t, FVM_SWFSoundVoice **);
void fvm_test_event_prepare_delay(void);
FVM_Result delayed_voice_open(FVM_Context *context, size_t ordinal, size_t budget,
                              FVM_SWFSoundVoice **out)
{
    fvm_test_event_prepare_delay();
    return fvm_swf_sound_voice_open_bounded(context, ordinal, budget, out);
}
