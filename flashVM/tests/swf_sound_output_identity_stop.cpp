/* Identity cancellation follows acknowledged clear and never mutates retained accepted PCM. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_fixture.hpp"

using namespace output_fixture;
using namespace conversion_fixture;
static FVM_SWFSoundVoice *voices[6];


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Samples samples;
    samples.resize(100);
    for (size_t i = 0; i < samples.size(); ++i)
    {
        samples[i] = 2000;
    }
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 6; ++i)
    {
        if (!(fvm_swf_sound_voice_open(c, i ? 1 : 0, &voices[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_SWFSoundMixer *mixer = create(48000, 2, 2);
    Device device;
    FVM_AudioQueueBackend table = backend(device);
    FVM_SWFSoundOutput *output = 0;
    if (!(fvm_swf_sound_output_create(mixer, &table, 16, 64, &output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.output = output;
    if (!(fvm_swf_sound_output_set_identity_tracking(output, 1) == FVM_OK &&
          fvm_swf_sound_output_set_release(output, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voices[0], (size_t)-1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t n = 0;
    if (!(fvm_swf_sound_output_pump(output, &n) == FVM_OK && n == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int playing = 0;
    device.failure = 3;
    if (!(fvm_swf_sound_output_stop(output) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 0;
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 4;
    if (!(fvm_swf_sound_output_stop(output) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 0;
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing && device.queued == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK && !device.queued))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && !playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voices[1], (size_t)-1) == FVM_OK && active(mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump(output, &n) == FVM_OK && fvm_swf_sound_output_pause(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    consume(device, 2);
    device.failure = 4;
    if (!(fvm_swf_sound_output_stop_release(output) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 0;
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 1;
    if (!(fvm_swf_sound_output_stop_release(output) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    device.failure = 0;
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && !playing && !active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundOutputInfo state = output_fixture::info(output);
    if (!(state.pending_frames == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voices[2], (size_t)-1) == FVM_OK && active(mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump(output, &n) == FVM_OK && n == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* Retry anonymous release first. */
    size_t before = device.values;
    short accepted[128];
    std::memcpy(accepted, device.history, before * sizeof(short));
    if (!(fvm_swf_sound_output_stop_id(output, 9) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop_id(output, 7) == FVM_OK && !active(mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_sound_active(output, 7, &playing) == FVM_OK && !playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(device.values == before && !std::memcmp(accepted, device.history, before * sizeof(short))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_submit_prepared_bounded(output, &voices[3], (size_t)-1) == FVM_OK && active(mixer) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop_id(output, 0) == FVM_INVALID_ARGUMENT &&
          fvm_swf_sound_output_stop_id(output, 65536) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(output) == FVM_OK &&
          fvm_swf_sound_output_set_identity_tracking(output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop_id(output, 7) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    for (unsigned i = 0; i < 6; ++i)
    {
        fvm_swf_sound_voice_close(voices[i]);
    }
    return 0;
}
