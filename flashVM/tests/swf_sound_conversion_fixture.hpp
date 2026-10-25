/* Independent absolute-time PCM oracle and raw SWF fixtures for output conversion. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_SOUND_CONVERSION_FIXTURE_HPP
#define FVM_TEST_SOUND_CONVERSION_FIXTURE_HPP
#include "flashvm/swf_sound_mixer.h"
#include "swf_sound_fixture.hpp"
#include <cmath>

namespace conversion_fixture
{
typedef sound_fixture::Bytes Bytes;
typedef fvm_detail::vector<short> Samples;
const unsigned rates[] = {5512, 11025, 22050, 44100};


inline void definition(Bytes &tags, unsigned id, unsigned rate_code, unsigned channels, const Samples &samples)
{
    Bytes pcm;
    for (size_t i = 0; i < samples.size(); ++i)
    {
        sound_fixture::word(pcm, (unsigned short)samples[i]);
    }
    sound_fixture::tag(tags,
                       14,
                       sound_fixture::sound(id,
                                            0x32 | (rate_code << 2) | (channels - 1),
                                            (unsigned long)(samples.size() / channels),
                                            pcm.begin(),
                                            pcm.size()));
}


inline void event(Bytes &tags, unsigned id, unsigned flags = 0, unsigned loops = 0)
{
    Bytes body;
    sound_fixture::word(body, id);
    body.push_back((unsigned char)flags);
    if (flags & FVM_SWF_SOUND_EVENT_LOOPS)
    {
        sound_fixture::word(body, loops);
    }
    sound_fixture::tag(tags, 15, body);
}


inline Bytes movie(const Samples &samples, unsigned rate_code, unsigned channels, unsigned loops = 1)
{
    Bytes tags;
    definition(tags, 7, rate_code, channels, samples);
    event(tags, 7, FVM_SWF_SOUND_EVENT_LOOPS, loops);
    event(tags, 7, FVM_SWF_SOUND_EVENT_NO_MULTIPLE);
    event(tags, 7, FVM_SWF_SOUND_EVENT_STOP);
    return sound_fixture::movie(tags);
}


inline FVM_SWFSoundMixer *create(unsigned rate, unsigned channels, unsigned capacity = 4)
{
    FVM_SWFSoundMixerConfig config = {sizeof(config), rate, channels, capacity, FVM_SWF_SOUND_MIXER_CONVERT};
    FVM_SWFSoundMixer *mixer = 0;
    if (!(fvm_swf_sound_mixer_create_configured(&config, &mixer) == FVM_OK && mixer))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return mixer;
}


inline unsigned active(FVM_SWFSoundMixer *mixer)
{
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(mixer, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return info.active_voices;
}


inline long
mapped(const Samples &samples, size_t at, unsigned source_channels, unsigned output_channels, unsigned channel)
{
    if (output_channels == 1 && source_channels == 2)
    {
        return (long)(((double)samples[at * 2] + samples[at * 2 + 1]) / 2.0);
    }
    return samples[at * source_channels + (source_channels == 1 ? 0 : channel)];
}


/* Compute each output independently from absolute output index, never reuse mixer phase state. */
inline long oracle(const Samples &samples,
                   unsigned rate,
                   unsigned source_channels,
                   unsigned output_rate,
                   unsigned output_channels,
                   size_t frame,
                   unsigned channel,
                   unsigned loops = 1)
{
    size_t source_frames = samples.size() / source_channels;
    size_t total = source_frames * loops;
    double numerator = (double)frame * rate;
    size_t at = (size_t)std::floor(numerator / output_rate);
    if (at >= total)
    {
        return 0;
    }
    unsigned long remainder = (unsigned long)std::fmod(numerator, output_rate);
    size_t next = at + 1 < total ? at + 1 : at;
    long left = mapped(samples, at % source_frames, source_channels, output_channels, channel);
    long right = mapped(samples, next % source_frames, source_channels, output_channels, channel);
    return (long)(((double)left * (output_rate - remainder) + (double)right * remainder) / output_rate);
}


inline size_t
duration(const Samples &samples, unsigned rate, unsigned channels, unsigned output_rate, unsigned loops = 1)
{
    return (size_t)std::ceil((double)(samples.size() / channels) * loops * output_rate / rate);
}
} // namespace conversion_fixture
#endif
