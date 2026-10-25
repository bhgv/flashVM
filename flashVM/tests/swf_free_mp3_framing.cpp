/* Free-format framing against original encoder packets, with padding and immutable error output. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_free_mp3_fixture.hpp"
#include "../src/swf/sound_mp3.hpp"
#include <cstring>


int main()
{
    for (size_t i = 0; i < sizeof(free_mp3_fixture::fixtures) / sizeof(free_mp3_fixture::fixtures[0]); ++i)
    {
        const free_mp3_fixture::Fixture &f = free_mp3_fixture::fixtures[i];
        FVM_SWFMP3Frame packet;
        if (!(fvm_swf_mp3_probe(f.data, f.size, &packet) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t base = (f.rate >= 32000 ? 144000UL : 72000UL) * f.kbps / f.rate;
        if (!(packet.bytes == base + ((f.data[2] & 2) != 0) && packet.rate == f.rate && packet.channels == f.channels &&
              packet.samples == f.packet_samples))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFMP3Frame saved = packet;
        if (!(fvm_swf_mp3_probe(f.data, packet.bytes, &packet) == FVM_UNSUPPORTED &&
              !std::memcmp(&saved, &packet, sizeof(packet))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char bad[4] = {0, 0, 0, 0};
        if (!(fvm_swf_mp3_probe(bad, 4, &packet) == FVM_INVALID_ARGUMENT &&
              !std::memcmp(&saved, &packet, sizeof(packet))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
