/* Independent zero-code partial packets and exact native PCM verification. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_INFERRED_ADPCM_FIXTURE_HPP
#define FVM_TEST_INFERRED_ADPCM_FIXTURE_HPP
#define main adpcm_reference_main
#include "swf_stream_adpcm.cpp"

#undef main


/* Bit fixture with independently predictable zero-magnitude codes and packet restarts. */
static Bits partial_bits(unsigned width, unsigned channels, size_t frames)
{
    Bits bits;
    bits.put(width - 2, 2);
    for (size_t f = 0; f < frames; ++f)
    {
        for (unsigned ch = 0; ch < channels; ++ch)
        {
            if (!(f % 4096))
            {
                bits.put(ch ? 65436 : 100, 16);
                bits.put(0, 6);
            }
            else
            {
                bits.put(0, width);
            }
        }
    }
    return bits;
}


/* Check every value after source teardown and exercise an incremental reader's exact EOF. */
static void verify_partial(FVM_SWFStreamSoundSnapshot *source, size_t frames, unsigned channels, unsigned width)
{
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    if (!(fvm_swf_stream_sound_snapshot_block_at(source, 0, &block) == FVM_OK && block.sample_count_known &&
          block.sample_count == frames && block.data_size == frames * channels * sizeof(short) && block.frame == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamPCMReader reader;
    if (!(fvm_swf_stream_pcm_reader_init(&reader, source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short pcm[62];
    size_t total = 0, written;
    for (;;)
    {
        FVM_SWFStreamPCMChunk chunk;
        chunk.struct_size = sizeof(chunk);
        FVM_Result result = fvm_swf_stream_pcm_reader_next(&reader, 31, pcm, 62, &chunk, &written);
        if (result == FVM_NOT_FOUND)
        {
            break;
        }
        if (!(result == FVM_OK && written == chunk.frames * channels))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < chunk.frames; ++i)
        {
            for (unsigned ch = 0; ch < channels; ++ch)
            {
                long expected = (ch ? -100L : 100L) + (long)((total + i) % 4096) * (7 / (1U << (width - 1)));
                if (!(pcm[i * channels + ch] == expected))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        total += chunk.frames;
    }
    if (!(total == frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}

#endif
