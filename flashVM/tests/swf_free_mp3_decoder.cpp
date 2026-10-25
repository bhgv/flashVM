/* Exact native frame boundaries, persistent reservoir and independent libmpg123 golden positions. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/swf/sound_mp3.hpp"
#include "swf_free_mp3_fixture.hpp"
#include <cstring>
#include <cstdlib>

using namespace free_mp3_fixture;


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
            if (!(fvm_swf_mp3_probe(fixture.data + at, 3, &packet) == FVM_INVALID_ARGUMENT &&
                  !std::memcmp(&packet, &untouched, sizeof(packet))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(decoder.decode(fixture.data + at, fixture.size - at, &packet) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(packet.rate == fixture.rate && packet.channels == fixture.channels))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(packet.bytes <= fixture.size - at))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(packet.samples == (fixture.rate >= 32000 ? 1152U : 576U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (size_t p = 0; p < sizeof(positions) / sizeof(positions[0]); ++p)
            {
                if (positions[p] >= frames && positions[p] - frames < packet.samples)
                {
                    for (unsigned ch = 0; ch < fixture.channels; ++ch)
                    {
                        long actual = decoder.pcm[(positions[p] - frames) * fixture.channels + ch];
                        long expected = fixture.golden[p * fixture.channels + ch];
                        if (!(std::labs(actual - expected) <= 2))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        ++checked;
                    }
                }
            }
            frames += packet.samples;
            at += packet.bytes;
        }
        if (!(at == fixture.size && frames == 5760 &&
              checked == sizeof(positions) / sizeof(positions[0]) * fixture.channels))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
