/* Independent PCM/control oracles: bounded polyphony, one clipping pass and native ownership. */
#include "flashvm/swf_sound_mixer.h"
#include "flashvm/gc.h"
#include "swf_sound_fixture.hpp"
#include <climits>
using namespace sound_fixture;

static void event(Bytes &defs, unsigned id, unsigned flags, unsigned loops = 0)
{
    Bytes data;
    word(data, id);
    data.push_back((unsigned char)flags);
    if (flags & FVM_SWF_SOUND_EVENT_LOOPS)
        word(data, loops);
    tag(defs, 15, data);
}

static Bytes fixture(unsigned channels, unsigned rate_code)
{
    Bytes defs;
    for (unsigned id = 7; id <= 9; ++id)
    {
        Bytes samples;
        for (unsigned frame = 0; frame < 3; ++frame)
            for (unsigned ch = 0; ch < channels; ++ch)
                word(samples, (unsigned short)((id == 9 ? -1 : 1) * (ch ? -30000 : 30000)));
        tag(defs, 14,
            sound(id, 0x32 | (rate_code << 2) | (channels - 1), 3, samples.begin(),
                  samples.size()));
    }
    event(defs, 7, 0); /* 0: ordinary first voice. */
    event(defs, 8, 0); /* 1: ordinary independent voice. */
    event(defs, 9, 0); /* 2: opposite polarity, exposes intermediate clipping. */
    event(defs, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE); /* 3 */
    event(defs, 7, FVM_SWF_SOUND_EVENT_STOP);        /* 4 */
    event(defs, 7, FVM_SWF_SOUND_EVENT_LOOPS, 0);    /* 5: consumes no slot. */
    event(defs, 7, FVM_SWF_SOUND_EVENT_LOOPS, 3);    /* 6 */
    event(defs, 8, FVM_SWF_SOUND_EVENT_STOP | FVM_SWF_SOUND_EVENT_NO_MULTIPLE); /* 7 */
    return movie(defs);
}

static unsigned active(FVM_SWFSoundMixer *mixer)
{
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    assert(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK);
    return info.active_voices;
}

static void mixture(FVM_Context *context, unsigned channels, unsigned rate)
{
    const unsigned orders[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                   {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (unsigned permutation = 0; permutation < 6; ++permutation)
    {
        FVM_SWFSoundMixer *mixer = 0;
        assert(fvm_swf_sound_mixer_create(rate, channels, 3, &mixer) == FVM_OK);
        for (unsigned j = 0; j < 3; ++j)
            assert(fvm_swf_sound_mixer_submit(mixer, context, orders[permutation][j]) == FVM_OK);
        assert(active(mixer) == 3);
        short output[16];
        output[0] = 123;
        size_t written = 99;
        assert(fvm_swf_sound_mixer_read(mixer, 4, output, 4 * channels - 1, &written) ==
               FVM_INVALID_ARGUMENT);
        assert(!written && output[0] == 123 && active(mixer) == 3);
        assert(fvm_swf_sound_mixer_read(mixer, (size_t)-1, output, 1, &written) ==
               FVM_INVALID_ARGUMENT);
        assert(fvm_swf_sound_mixer_read(mixer, 0, 0, 0, &written) == FVM_OK && !written);
        assert(fvm_swf_sound_mixer_read(mixer, 4, output, 16, &written) == FVM_OK &&
               written == 4 * channels);
        for (unsigned frame = 0; frame < 4; ++frame)
            for (unsigned ch = 0; ch < channels; ++ch)
                assert(output[frame * channels + ch] == (frame == 3 ? 0 : ch ? -30000 : 30000));
        assert(!active(mixer));
        /* An exhausted no-multiple source is eligible again. */
        assert(fvm_swf_sound_mixer_submit(mixer, context, 3) == FVM_OK && active(mixer) == 1);
        assert(fvm_swf_sound_mixer_stop_all(mixer) == FVM_OK && !active(mixer));
        fvm_swf_sound_mixer_close(mixer);
    }
}

static void controls(FVM_Context *context)
{
    FVM_SWFSoundMixer *mixer = 0;
    assert(fvm_swf_sound_mixer_create(44100, 1, 3, &mixer) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 1) == FVM_OK && active(mixer) == 3);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 3) == FVM_OK && active(mixer) == 3);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 5) == FVM_OK && active(mixer) == 3);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 2) == FVM_OUT_OF_MEMORY &&
           active(mixer) == 3);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 99) == FVM_NOT_FOUND && active(mixer) == 3);
    /* ID-specific stop removes both copies of ID 7 and leaves ID 8 intact. */
    assert(fvm_swf_sound_mixer_submit(mixer, context, 4) == FVM_OK && active(mixer) == 1);
    short output[5];
    size_t written = 0;
    assert(fvm_swf_sound_mixer_read(mixer, 1, output, 5, &written) == FVM_OK && output[0] == 30000);
    /* Stop wins when both synchronization flags are set. */
    assert(fvm_swf_sound_mixer_submit(mixer, context, 7) == FVM_OK && !active(mixer));
    assert(fvm_swf_sound_mixer_read(mixer, 5, output, 5, &written) == FVM_OK && written == 5);
    for (unsigned i = 0; i < 5; ++i)
        assert(!output[i]);
    fvm_swf_sound_mixer_close(mixer);
    /* Geometry rejection must neither decode nor perturb an existing voice. */
    assert(fvm_swf_sound_mixer_create(22050, 1, 1, &mixer) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_UNSUPPORTED && !active(mixer));
    assert(fvm_swf_sound_mixer_submit(mixer, context, 4) == FVM_OK);
    fvm_swf_sound_mixer_close(mixer);
}

int main()
{
    FVM_SWFSoundMixer *mixer = (FVM_SWFSoundMixer *)1;
    assert(fvm_swf_sound_mixer_create(123, 1, 1, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create(44100, 0, 1, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create(44100, 3, 1, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create(44100, 1, 0, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create(44100, 1, 65, &mixer) == FVM_INVALID_ARGUMENT && !mixer);
    assert(fvm_swf_sound_mixer_create(44100, 1, 1, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_mixer_stop_all(0) == FVM_INVALID_ARGUMENT);
    short output[600];
    size_t written = 99;
    assert(fvm_swf_sound_mixer_read(0, 1, output, 1, &written) == FVM_INVALID_ARGUMENT && !written);
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_swf_sound_mixer_create(44100, 1, 64, &mixer) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_UNSUPPORTED);
    fvm_swf_sound_mixer_close(mixer);
    const unsigned rates[] = {5512, 11025, 22050, 44100};
    for (unsigned code = 0; code < 4; ++code)
        for (unsigned channels = 1; channels <= 2; ++channels)
        {
            Bytes swf = fixture(channels, code);
            assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
            mixture(context, channels, rates[code]);
        }
    Bytes swf = fixture(1, 3);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    controls(context);
    /* Source reload changes geometry but never changes prepared source samples. */
    assert(fvm_swf_sound_mixer_create(44100, 1, 2, &mixer) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    swf = fixture(1, 2);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_UNSUPPORTED && active(mixer) == 1);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 3) == FVM_OK && active(mixer) == 1);
    assert(fvm_swf_sound_mixer_read(mixer, 1, output, 600, &written) == FVM_OK &&
           output[0] == 30000);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 4) == FVM_OK && !active(mixer));
    fvm_swf_sound_mixer_close(mixer);
    swf = fixture(1, 3);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    assert(fvm_swf_sound_mixer_create(44100, 1, 64, &mixer) == FVM_OK);
    for (unsigned i = 0; i < 64; ++i)
        assert(fvm_swf_sound_mixer_submit(mixer, context, 6) == FVM_OK);
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    Bytes empty;
    swf = movie(empty);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    assert(fvm_swf_sound_mixer_read(mixer, 600, output, 600, &written) == FVM_OK && written == 600);
    for (unsigned i = 0; i < 600; ++i)
        assert(output[i] == (i < 9 ? 32767 : 0));
    assert(!active(mixer));
    FVM_SWFSoundMixerInfo info;
    std::memset(&info, 0x7e, sizeof(info));
    FVM_SWFSoundMixerInfo before = info;
    assert(fvm_swf_sound_mixer_info(mixer, &info) == FVM_INVALID_ARGUMENT &&
           !std::memcmp(&info, &before, sizeof(info)));
    fvm_swf_sound_mixer_close(mixer);
    fvm_swf_sound_mixer_close(0);
    return 0;
}
