/* Portable mixing invokes neither allocation nor host/VM operations. */
#define main mixer_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/multimedia.h"


int main()
{
    short data[] = {32767, -32768}, out[2] = {0};
    FVM_MediaFrame f, result;
    std::memset(&f, 0, sizeof(f));
    f.kind = FVM_MEDIA_AUDIO;
    f.sample_rate = 44100;
    f.channels = 1;
    f.samples = 2;
    f.size = sizeof(data);
    f.data = (const unsigned char *)data;
    allowance = 0;
    if (!(fvm_audio_mix_s16(&f, 1, 44100, 1, 2, out, 2, &result) == FVM_OK && out[0] == 32767 && out[1] == -32768))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    out[0] = 42;
    if (!(fvm_audio_mix_s16(&f, 1, 44100, 1, 2, out, 1, &result) == FVM_INVALID_ARGUMENT && out[0] == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_mix_s16(0, 0, 44100, 1, 2, out, 2, &result) == FVM_OK && !out[0] && !out[1]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    return 0;
}
