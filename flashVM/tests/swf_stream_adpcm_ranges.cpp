/* Allocation-free authoritative ADPCM ranges: live source, encoded snapshot and complete validation. */
#define main adpcm_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_adpcm.cpp"

#undef main
#include <climits>


/* Assert that failed calls preserve all destination values and clear only the written count. */
static void failure(FVM_SWFStreamSoundSnapshot *source,
                    size_t samples,
                    size_t first,
                    size_t frames,
                    size_t capacity,
                    FVM_Result expected)
{
    short pcm[4] = {111, 222, 333, 444};
    size_t written = 91;
    if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, samples, first, frames, pcm, capacity, &written) ==
              expected &&
          !written))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pcm[0] == 111 && pcm[1] == 222 && pcm[2] == 333 && pcm[3] == 444))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    for (unsigned width = 2; width <= 5; ++width)
    {
        for (unsigned channels = 1; channels <= 2; ++channels)
        {
            const size_t samples = 4099;
            Bits bits;
            bits.put(width - 2, 2);
            for (size_t f = 0; f < samples; ++f)
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
            FVM_Config cfg;
            FVM_Runtime *rt = 0;
            FVM_Context *c = 0;
            if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
                  fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes swf = swf_block(bits.bytes, channels);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamSoundSnapshot *source = 0, *normalized = 0;
            if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &source) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_snapshot_open_adpcm_counted(c, 0, &samples, 1, (size_t)-1, &normalized) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            size_t written = 91;
            short pcm[10];
            const size_t starts[] = {0, 1, 17, 4094, 4096, 4098, 4099};
            for (size_t i = 0; i < sizeof(starts) / sizeof(starts[0]); ++i)
            {
                size_t first = starts[i], frames = samples - first;
                if (frames > 5)
                {
                    frames = 5;
                }
                if (!(fvm_swf_stream_sound_read_adpcm(c, 0, 0, samples, first, frames, pcm, 10, &written) == FVM_OK &&
                      written == frames * channels))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (size_t f = 0; f < frames; ++f)
                {
                    for (unsigned ch = 0; ch < channels; ++ch)
                    {
                        if (!(pcm[f * channels + ch] ==
                              (ch ? -100L : 100L) + (long)((first + f) % 4096) * (7 / (1U << (width - 1)))))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            failure(source, samples, samples + 1, 0, 4, FVM_INVALID_ARGUMENT);
            failure(source, samples, samples - 1, 2, 4, FVM_INVALID_ARGUMENT);
            failure(source, samples, 0, 5, 4, FVM_INVALID_ARGUMENT);
            failure(normalized, samples, 0, 1, 4, FVM_UNSUPPORTED);
            failure(0, samples, 0, 1, 4, FVM_INVALID_ARGUMENT);
            if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, samples, 0, 1, 0, 10, &written) ==
                      FVM_INVALID_ARGUMENT &&
                  !written))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, samples, 0, 0, 0, 0, &written) == FVM_OK &&
                  !written))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, samples, 0, 0, 0, 0, 0) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_read_adpcm(c, 0, 0, samples, 0, 0, 0, 0, 0) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_read_adpcm(0, 0, 0, samples, 0, 0, 0, 0, &written) == FVM_INVALID_ARGUMENT &&
                  !written))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 1, samples, 0, 0, 0, 0, &written) == FVM_NOT_FOUND &&
                  !written))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* A bad suffix invalidates even an early or empty requested range. */
            Bytes bad = bits.bytes;
            bad.push_back(0);
            swf = swf_block(bad, channels);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_read_adpcm(c, 0, 0, samples, 0, 1, pcm, 10, &written) == FVM_INVALID_ARGUMENT &&
                  !written))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_stream_sound_read_adpcm(c, 0, 0, samples, 0, 0, 0, 0, &written) == FVM_INVALID_ARGUMENT &&
                  !written))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_SWFStreamSoundSnapshot *malformed = 0;
            if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &malformed) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            failure(malformed, samples, 0, 1, 4, FVM_INVALID_ARGUMENT);
            fvm_swf_stream_sound_snapshot_close(malformed);
            fvm_context_destroy(c);
            fvm_runtime_destroy(rt);
            /* Raw encoded ownership and each output are still valid after source replacement/teardown. */
            if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, samples, 4094, 5, pcm, 10, &written) == FVM_OK &&
                  written == 5 * channels))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (size_t f = 0; f < 5; ++f)
            {
                for (unsigned ch = 0; ch < channels; ++ch)
                {
                    if (!(pcm[f * channels + ch] ==
                          (ch ? -100L : 100L) + (long)((4094 + f) % 4096) * (7 / (1U << (width - 1)))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            fvm_swf_stream_sound_snapshot_close(source);
            fvm_swf_stream_sound_snapshot_close(normalized);
        }
    }
    return 0;
}
