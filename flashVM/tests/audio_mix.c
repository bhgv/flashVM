/* C89 exact sums, final-only clipping, stereo/tail padding and atomic invalid-input handling. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/multimedia.h"
#include <string.h>


static FVM_MediaFrame view(const short *p, size_t count, unsigned channels)
{
    FVM_MediaFrame f;
    memset(&f, 0, sizeof(f));
    f.kind = FVM_MEDIA_AUDIO;
    f.sample_rate = 44100;
    f.channels = channels;
    f.samples = count;
    f.size = count * channels * sizeof(short);
    f.data = (const unsigned char *)p;
    return f;
}


int main(void)
{
    short a[] = {30000, -30000, 1000, -1000}, b[] = {10000, -10000, -1000, 1000}, pcm[8] = {99};
    FVM_MediaFrame inputs[3], out;
    size_t i;
    inputs[0] = view(a, 4, 1);
    inputs[1] = view(b, 4, 1);
    if (!(fvm_audio_mix_s16(inputs, 2, 44100, 1, 4, pcm, 8, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pcm[0] == 32767 && pcm[1] == -32768 && !pcm[2] && !pcm[3]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.kind == FVM_MEDIA_AUDIO && out.data == (const unsigned char *)pcm && out.samples == 4 && out.size == 8 &&
          !out.has_timestamp && !out.width))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        short one = 32767, two = 32767, three = -32768;
        FVM_MediaFrame ordered[3];
        unsigned permutation[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
        unsigned j;
        inputs[0] = view(&one, 1, 1);
        inputs[1] = view(&two, 1, 1);
        inputs[2] = view(&three, 1, 1);
        for (j = 0; j < 6; ++j)
        {
            for (i = 0; i < 3; ++i)
            {
                ordered[i] = inputs[permutation[j][i]];
            }
            if (!(fvm_audio_mix_s16(ordered, 3, 44100, 1, 1, pcm, 8, &out) == FVM_OK && pcm[0] == 32766))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    {
        short left[] = {32000, -32000, 100, -100}, right[] = {32000, -32000};
        inputs[0] = view(left, 2, 2);
        inputs[1] = view(right, 1, 2);
        if (!(fvm_audio_mix_s16(inputs, 2, 44100, 2, 2, pcm, 8, &out) == FVM_OK && pcm[0] == 32767 &&
              pcm[1] == -32768 && pcm[2] == 100 && pcm[3] == -100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, 3, pcm, 8, &out) == FVM_OK && !pcm[0] && !pcm[1] && !pcm[2]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, 0, 0, 0, &out) == FVM_OK && !out.data && !out.size && !out.samples))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inputs[0] = view(a, 4, 1);
    inputs[1] = view(0, 0, 1);
    if (!(fvm_audio_mix_s16(inputs, 2, 44100, 1, 4, pcm, 8, &out) == FVM_OK && !memcmp(a, pcm, sizeof(a))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* No destination publication can precede complete metadata validation. */
    for (i = 0; i < 7; ++i)
    {
        inputs[0] = view(a, 4, 1);
        inputs[1] = view(b, 4, 1);
        if (i == 0)
        {
            inputs[1].sample_rate = 22050;
        }
        if (i == 1)
        {
            inputs[1].channels = 2;
        }
        if (i == 2)
        {
            inputs[1].kind = FVM_MEDIA_VIDEO;
        }
        if (i == 3)
        {
            inputs[1].data = 0;
        }
        if (i == 4)
        {
            inputs[1].size = 1;
        }
        if (i == 5)
        {
            inputs[1].size = 6;
        }
        if (i == 6)
        {
            inputs[1].samples = 0;
            inputs[1].size = 1;
        }
        out.width = 77;
        pcm[0] = 99;
        if (!(fvm_audio_mix_s16(inputs, 2, 44100, 1, 4, pcm, 8, &out) == FVM_INVALID_ARGUMENT && pcm[0] == 99 &&
              out.width == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_audio_mix_s16(0, 1, 44100, 1, 1, pcm, 8, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 0, 1, 1, pcm, 8, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 0, 1, pcm, 8, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, 1, 0, 8, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, 1, pcm, 0, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, (size_t)-1, pcm, (size_t)-1, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, 1, pcm, 8, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(inputs, FVM_MEDIA_AUDIO_MIX_MAX_INPUTS + 1, 44100, 1, 1, pcm, 8, &out) ==
          FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Exact-start payload aliasing and unaligned input bytes remain safe. */
    inputs[0] = view(a, 4, 1);
    inputs[1] = view(b, 4, 1);
    if (!(fvm_audio_mix_s16(inputs, 2, 44100, 1, 4, a, 4, &out) == FVM_OK && a[0] == 32767 && a[1] == -32768 && !a[2]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        unsigned char unaligned[5];
        short raw[] = {123, -456};
        memcpy(unaligned + 1, raw, 4);
        inputs[0] = view(raw, 2, 1);
        inputs[0].data = unaligned + 1;
        if (!(fvm_audio_mix_s16(inputs, 1, 44100, 1, 2, pcm, 8, &out) == FVM_OK && pcm[0] == 123 && pcm[1] == -456))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        FVM_MediaFrame many[FVM_MEDIA_AUDIO_MIX_MAX_INPUTS];
        short value = 32767;
        for (i = 0; i < FVM_MEDIA_AUDIO_MIX_MAX_INPUTS; ++i)
        {
            many[i] = view(&value, 1, 1);
        }
        if (!(fvm_audio_mix_s16(many, FVM_MEDIA_AUDIO_MIX_MAX_INPUTS, 44100, 1, 1, pcm, 8, &out) == FVM_OK &&
              pcm[0] == 32767))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = -32768;
        if (!(fvm_audio_mix_s16(many, FVM_MEDIA_AUDIO_MIX_MAX_INPUTS, 44100, 1, 1, pcm, 8, &out) == FVM_OK &&
              pcm[0] == -32768))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
