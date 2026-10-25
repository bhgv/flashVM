/* Seed exact epoch limits; finite short work is checked before rendering and never overflows. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/swf/sound_output.cpp"
#include "swf_sound_output_finite_fixture.hpp"


int main()
{
    FiniteOutput f;
    f.start();
    short pcm[16];
    size_t n = 0;
    if (!(fvm_swf_sound_mixer_read(f.mixer, 8, pcm, 16, &n) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.output->info.submitted_frames = f.output->info.played_frames = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 1;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK && n == 1 &&
          f.output->info.submitted_frames == FVM_SWF_SOUND_OUTPUT_MAX_CLOCK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.queued = 0;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.start();
    size_t remaining = 0;
    if (!(fvm_swf_sound_mixer_active_frames(f.mixer, 64, &remaining) == FVM_OK && remaining == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mixer_active_frames(f.mixer, 64, &remaining) == FVM_OK && remaining == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(f.output) == FVM_OK && !f.output->info.submitted_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
