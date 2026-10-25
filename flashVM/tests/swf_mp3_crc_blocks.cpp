/* Count/length-derived free-format inference, grouped packets, padding and atomic bad metadata. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_crc_mp3_fixture.hpp"
#include "../src/swf/sound_mp3.hpp"
#include <cstring>
#include <cstdlib>


int main()
{
    for (size_t f = 0; f < sizeof(crc_mp3_fixture::fixtures) / sizeof(crc_mp3_fixture::fixtures[0]); ++f)
    {
        const crc_mp3_fixture::Fixture &v = crc_mp3_fixture::fixtures[f];
        size_t base = (v.rate >= 32000 ? 144000UL : 72000UL) * v.kbps / v.rate;
        for (unsigned group = 1; group <= 5; ++group)
        {
            FVM_SWFMP3Decoder decoder;
            size_t at = 0, total = 0;
            while (at < v.size)
            {
                size_t end = at;
                unsigned packets = 0;
                while (end < v.size && packets < group)
                {
                    end += base + ((v.data[end + 2] & 2) != 0);
                    ++packets;
                }
                if (!(end <= v.size))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(decoder.prepare_block(v.data + at, end - at, packets * v.packet_samples) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                while (at < end)
                {
                    FVM_SWFMP3Frame info;
                    if (!(decoder.decode(v.data + at, end - at, &info) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (size_t p = 0; p < sizeof(crc_mp3_fixture::positions) / sizeof(crc_mp3_fixture::positions[0]);
                         ++p)
                    {
                        if (crc_mp3_fixture::positions[p] >= total &&
                            crc_mp3_fixture::positions[p] - total < info.samples)
                        {
                            for (unsigned ch = 0; ch < v.channels; ++ch)
                            {
                                if (!(std::labs(
                                          (long)decoder.pcm[(crc_mp3_fixture::positions[p] - total) * v.channels + ch] -
                                          v.golden[p * v.channels + ch]) <= 2))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                    }
                    at += info.bytes;
                    total += info.samples;
                }
            }
            if (!(total == 5760))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if ((v.data[2] >> 4) != 0)
        {
            continue;
        }
        FVM_SWFMP3Decoder decoder;
        mp3dec_t saved = decoder.state;
        unsigned char bad[10000];
        if (!(v.size <= sizeof(bad)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        std::memcpy(bad, v.data, v.size);
        for (size_t at = 0; at < v.size;)
        {
            bad[at + 4] ^= 1;
            if (!(decoder.prepare_block(bad, v.size, 5760) == FVM_INVALID_ARGUMENT &&
                  !std::memcmp(&saved, &decoder.state, sizeof(saved))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bad[at + 4] ^= 1;
            at += base + ((v.data[at + 2] & 2) != 0);
        }
        if (!(decoder.prepare_block(v.data, v.size, 1) == FVM_INVALID_ARGUMENT &&
              !std::memcmp(&saved, &decoder.state, sizeof(saved))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoder.prepare_block(v.data, v.size, 0) == FVM_INVALID_ARGUMENT &&
              !std::memcmp(&saved, &decoder.state, sizeof(saved))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoder.prepare_block(v.data, v.size, 65536U) == FVM_INVALID_ARGUMENT &&
              !std::memcmp(&saved, &decoder.state, sizeof(saved))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoder.prepare_block(v.data, v.size - 1, 5760) == FVM_INVALID_ARGUMENT &&
              !std::memcmp(&saved, &decoder.state, sizeof(saved))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoder.prepare_block(0, 0, 0) == FVM_OK && !std::memcmp(&saved, &decoder.state, sizeof(saved))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoder.prepare_block(v.data, v.size, 5760) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
