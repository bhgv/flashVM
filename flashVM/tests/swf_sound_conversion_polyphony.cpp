/* Different source clocks/channels share one output clock; clipping is independent of voice order.
 */
#include "swf_sound_conversion_fixture.hpp"
using namespace conversion_fixture;
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    Samples source[3];
    const unsigned codes[] = {0, 2, 3}, channels[] = {1, 2, 1};
    Bytes tags;
    for (unsigned voice = 0; voice < 3; ++voice)
    {
        for (unsigned i = 0; i < 17 * channels[voice]; ++i)
            source[voice].push_back(voice == 2 ? -30000 : 30000);
        definition(tags, 7 + voice, codes[voice], channels[voice], source[voice]);
    }
    for (unsigned i = 0; i < 3; ++i)
        event(tags, 7 + i);
    Bytes swf = sound_fixture::movie(tags);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    const unsigned order[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (unsigned permutation = 0; permutation < 6; ++permutation)
    {
        FVM_SWFSoundMixer *mixer = create(48000, 2, 3);
        for (unsigned i = 0; i < 3; ++i)
            assert(fvm_swf_sound_mixer_submit(mixer, context, order[permutation][i]) == FVM_OK);
        short output[514];
        size_t written = 0;
        assert(fvm_swf_sound_mixer_read(mixer, 257, output, 514, &written) == FVM_OK &&
               written == 514);
        for (size_t frame = 0; frame < 257; ++frame)
        {
            long sum = 0;
            for (unsigned i = 0; i < 3; ++i)
                sum += oracle(source[i], rates[codes[i]], channels[i], 48000, 2, frame, 0);
            if (sum > 32767)
                sum = 32767;
            if (sum < -32768)
                sum = -32768;
            assert(output[frame * 2] == sum && output[frame * 2 + 1] == sum);
        }
        assert(!active(mixer));
        fvm_swf_sound_mixer_close(mixer);
    }
    Samples negative;
    negative.push_back(-32768);
    swf = movie(negative, 3, 1);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFSoundMixer *mixer = create(48000, 2, 64);
    for (unsigned i = 0; i < 64; ++i)
        assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    short output[6];
    size_t written = 0;
    assert(fvm_swf_sound_mixer_read(mixer, 3, output, 6, &written) == FVM_OK && written == 6);
    assert(output[0] == -32768 && output[1] == -32768 && output[2] == -32768 &&
           output[3] == -32768 && !output[4] && !output[5]);
    fvm_swf_sound_mixer_close(mixer);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
