/* Exact native frame boundaries, persistent reservoir and independent FFmpeg golden positions. */
#include "../src/swf/sound_mp3.hpp"
#include "swf_mp3_fixture.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
using namespace mp3_fixture;
int main()
{
    for (size_t index = 0; index < sizeof(fixtures) / sizeof(fixtures[0]); ++index)
    {
        const Fixture &fixture = fixtures[index];
        FVM_SWFMP3Decoder decoder;
        size_t at = 0, frames = 0, checked = 0;
        while (at < fixture.size)
        {
            FVM_SWFMP3Frame packet, untouched;
            std::memset(&untouched, 77, sizeof(untouched));
            packet = untouched;
            assert(fvm_swf_mp3_probe(fixture.data + at, 3, &packet) == FVM_INVALID_ARGUMENT &&
                   !std::memcmp(&packet, &untouched, sizeof(packet)));
            assert(decoder.decode(fixture.data + at, fixture.size - at, &packet) == FVM_OK);
            assert(packet.rate == fixture.rate && packet.channels == fixture.channels);
            assert(packet.bytes <= fixture.size - at);
            assert(packet.samples == (fixture.rate >= 32000 ? 1152U : 576U));
            for (size_t p = 0; p < sizeof(positions) / sizeof(positions[0]); ++p)
                if (positions[p] >= frames && positions[p] - frames < packet.samples)
                    for (unsigned ch = 0; ch < fixture.channels; ++ch)
                    {
                        long actual = decoder.pcm[(positions[p] - frames) * fixture.channels + ch];
                        long expected = fixture.golden[p * fixture.channels + ch];
                        assert(std::labs(actual - expected) <= 2);
                        ++checked;
                    }
            frames += packet.samples;
            at += packet.bytes;
        }
        assert(at == fixture.size && frames == fixture.frames && checked ==
               sizeof(positions) / sizeof(positions[0]) * fixture.channels);
        FVM_SWFMP3Frame packet;
        assert(fvm_swf_mp3_probe(fixture.data, fixture.size, &packet) == FVM_OK);
        assert(fvm_swf_mp3_probe(fixture.data, packet.bytes - 1, &packet) == FVM_INVALID_ARGUMENT);
        unsigned char bad[2048];
        assert(packet.bytes <= sizeof(bad));
        std::memcpy(bad, fixture.data, packet.bytes);
        bad[1] = (unsigned char)((bad[1] & ~24U) | 8U);
        assert(fvm_swf_mp3_probe(bad, packet.bytes, &packet) == FVM_INVALID_ARGUMENT);
        std::memcpy(bad, fixture.data, packet.bytes); bad[2] &= 15;
        assert(fvm_swf_mp3_probe(bad, packet.bytes, &packet) == FVM_UNSUPPORTED);
        std::memcpy(bad, fixture.data, packet.bytes); bad[2] |= 240;
        assert(fvm_swf_mp3_probe(bad, packet.bytes, &packet) == FVM_INVALID_ARGUMENT);
        std::memcpy(bad, fixture.data, packet.bytes); bad[2] |= 12;
        assert(fvm_swf_mp3_probe(bad, packet.bytes, &packet) == FVM_INVALID_ARGUMENT);
        assert(decoder.decode(fixture.data, fixture.size, 0) == FVM_INVALID_ARGUMENT);
    }
    return 0;
}
