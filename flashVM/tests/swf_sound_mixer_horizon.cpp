/* Live converted lookahead has a finite horizon even after the underlying voice reaches EOF. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    GateFixture f;
    size_t n = 91;
    if (!(fvm_swf_sound_mixer_active_frames(f.mixer, 0, &n) == FVM_INVALID_ARGUMENT && n == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_active_frames(f.mixer, FVM_SWF_SOUND_MIXER_MAX_HORIZON + 1UL, &n) ==
              FVM_INVALID_ARGUMENT &&
          n == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_active_frames(f.mixer, 64, &n) == FVM_OK && n == 14))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short pcm[4];
    size_t written = 0;
    for (size_t frame = 0; frame < 14; ++frame)
    {
        if (!(fvm_swf_sound_mixer_active_frames(f.mixer, 64, &n) == FVM_OK && n == 14 - frame))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_mixer_read(f.mixer, 1, pcm, 4, &written) == FVM_OK && written == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_sound_mixer_active_frames(f.mixer, 64, &n) == FVM_OK && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
