/* Public SWF4-8 event MP3 extraction: effective geometry, latency, complete validation and PCM bounds. */
#include "swf_sound_fixture.hpp"
#include "swf_mp3_fixture.hpp"
#include <cstdlib>
using namespace sound_fixture;
/* Deliberately wrong advisory flags must not alter MPEG rate/channel decoding. */
static Bytes mp3_movie(const mp3_fixture::Fixture &fixture, unsigned seek, unsigned version = 8,
                        unsigned long declared = 0)
{
    Bytes payload, tags;
    word(payload, seek);
    for (size_t i = 0; i < fixture.size; ++i) payload.push_back(fixture.data[i]);
    if (!declared) declared = (unsigned long)(fixture.frames - seek);
    tag(tags, 14, sound(7, 0x22, declared, payload.begin(), payload.size()));
    return movie(tags, version);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
        for (size_t f = 0; f < sizeof(mp3_fixture::fixtures)/sizeof(mp3_fixture::fixtures[0]); ++f)
        {
            const mp3_fixture::Fixture &fixture = mp3_fixture::fixtures[f];
            for (unsigned seek = 0; seek <= 529; seek += 529)
            {
                Bytes swf = mp3_movie(fixture, seek, version);
                assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
                FVM_SWFSoundInfo info; info.struct_size = sizeof(info);
                assert(fvm_swf_sound_info(c, 7, &info) == FVM_OK && info.format == FVM_SWF_SOUND_MP3 &&
                       info.sample_rate == fixture.rate && info.channels == fixture.channels &&
                       info.bits_per_sample == 16 && info.sample_count == fixture.frames - seek);
                short pcm[4] = {123, 456, 789, 321}; size_t written = 91;
                for (size_t p = 0; p < sizeof(mp3_fixture::positions)/sizeof(mp3_fixture::positions[0]); ++p)
                {
                    size_t position = mp3_fixture::positions[p];
                    if (position < seek) continue;
                    assert(fvm_swf_sound_read_pcm(c, 7, (unsigned long)(position - seek), 1,
                                                   pcm, 4, &written) == FVM_OK && written == fixture.channels);
                    for (unsigned ch = 0; ch < fixture.channels; ++ch)
                        assert(std::labs((long)pcm[ch] - fixture.golden[p*fixture.channels+ch]) <= 2);
                }
                assert(fvm_swf_sound_read_pcm(c, 7, info.sample_count, 0, 0, 0, &written) == FVM_OK && !written);
                assert(fvm_swf_sound_read_pcm(c, 7, info.sample_count, 1, pcm, 4, &written) == FVM_INVALID_ARGUMENT && !written);
                assert(fvm_swf_sound_read_pcm(c, 7, 0, 1, pcm, 0, &written) == FVM_INVALID_ARGUMENT && !written);
                /* A malformed final byte range must reject even an early or empty read. */
                Bytes bad = swf;
                bad[bad.size()-5] = 255; /* Last MPEG byte can be arbitrary; use a short tag payload below. */
                Bytes payload, tags; word(payload, seek);
                for (size_t i = 0; i + 1 < fixture.size; ++i) payload.push_back(fixture.data[i]);
                tag(tags, 14, sound(7, 0x22, info.sample_count, payload.begin(), payload.size()));
                bad = movie(tags, version);
                assert(fvm_load_memory(c, bad.begin(), bad.size()) == FVM_OK);
                pcm[0] = 123;
                assert(fvm_swf_sound_read_pcm(c, 7, 0, 1, pcm, 4, &written) == FVM_INVALID_ARGUMENT && !written && pcm[0] == 123);
                assert(fvm_swf_sound_read_pcm(c, 7, 0, 0, 0, 0, &written) == FVM_INVALID_ARGUMENT && !written);
            }
            Bytes swf = mp3_movie(fixture, 0, version, (unsigned long)fixture.frames+1);
            assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
            size_t written = 91;
            assert(fvm_swf_sound_read_pcm(c, 7, 0, 0, 0, 0, &written) == FVM_INVALID_ARGUMENT && !written);
            swf = mp3_movie(fixture, 65535, version, 1);
            assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
            assert(fvm_swf_sound_read_pcm(c, 7, 0, 0, 0, 0, &written) == FVM_UNSUPPORTED && !written);
        }
    Bytes old = mp3_movie(mp3_fixture::fixtures[0], 0, 3);
    assert(fvm_load_memory(c, old.begin(), old.size()) == FVM_OK);
    size_t written = 91;
    assert(fvm_swf_sound_read_pcm(c, 7, 0, 0, 0, 0, &written) == FVM_UNSUPPORTED && !written);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
    return 0;
}
