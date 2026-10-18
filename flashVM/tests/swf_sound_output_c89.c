/* Strict C89 queue table, empty priming, pause/dequeue clock, stop and independent close. */
#include "flashvm/swf_sound_output.h"
#include <assert.h>
typedef struct Device
{
    size_t count;
    int paused;
} Device;
static FVM_Result write_pcm(void *raw, const short *pcm, size_t frames)
{
    Device *d = (Device *)raw;
    (void)pcm;
    d->count += frames;
    return FVM_OK;
}
static FVM_Result queued(void *raw, size_t *frames)
{
    *frames = ((Device *)raw)->count;
    return FVM_OK;
}
static FVM_Result pause_pcm(void *raw, int paused)
{
    ((Device *)raw)->paused = paused;
    return FVM_OK;
}
static FVM_Result clear_pcm(void *raw)
{
    ((Device *)raw)->count = 0;
    return FVM_OK;
}
int main(void)
{
    Device device = {0, 1};
    FVM_AudioQueueBackend backend = {sizeof(backend), FVM_AUDIO_QUEUE_ABI_VERSION,
                                     44100,           1,
                                     0,               write_pcm,
                                     queued,          pause_pcm,
                                     clear_pcm};
    FVM_SWFSoundMixer *mixer = 0;
    FVM_SWFSoundOutput *output = 0;
    FVM_SWFSoundOutputInfo info;
    size_t accepted = 0;
    backend.userdata = &device;
    assert(fvm_swf_sound_mixer_create(44100, 1, 1, &mixer) == FVM_OK);
    assert(fvm_swf_sound_output_create(mixer, &backend, 4, 8, &output) == FVM_OK);
    assert(fvm_swf_sound_output_set_release(output, 2) == FVM_OK);
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4);
    assert(fvm_swf_sound_output_pause(output, 0) == FVM_OK && !device.paused);
    device.count = 1;
    info.struct_size = sizeof(info);
    assert(fvm_swf_sound_output_info(output, &info) == FVM_OK && info.played_frames == 3);
    assert(fvm_swf_sound_output_stop(output) == FVM_OK && device.paused && !device.count);
    assert(fvm_swf_sound_output_stop_release(output) == FVM_OK && device.paused && !device.count);
    assert(fvm_swf_sound_output_set_release(output, 0) == FVM_OK);
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
