/* Pending observes native voice, retry and FIFO stages without producing or consuming PCM. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_finite_fixture.hpp"


int main()
{
    FiniteOutput f;
    f.device.reenter = true;
    int pending = 91;
    size_t n = 0;
    if (!(fvm_swf_sound_output_pending(0, &pending) == FVM_INVALID_ARGUMENT && pending == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.start();
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && pending && !f.device.values))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.failure = 1;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && pending && !f.device.values))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.failure = 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        f.device.queued = 0;
        if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!conversion_fixture::active(f.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.failure = 2;
    pending = 91;
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_ERROR && pending == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.failure = 0;
    f.device.queued = 0;
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output_fixture::info(f.output).played_frames == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
