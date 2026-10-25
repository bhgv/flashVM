/* Finite pumps retain ordinary and anonymous stop-release retries across errors and recursion. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_finite_fixture.hpp"


int main(int argc, char **argv)
{
    bool exceptions = argc == 1;
    if (!(argc == 1 || (argc == 2 && !std::strcmp(argv[1], "--no-throw"))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FiniteOutput f;
    f.device.reenter = true;
    if (!(fvm_swf_sound_output_set_release(f.output, 3) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.start();
    size_t n = 91;
    f.device.failure = 1;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_ERROR && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (exceptions)
    {
        for (int mode = 5; mode <= 6; ++mode)
        {
            f.device.failure = mode;
            if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == (mode == 5 ? FVM_OUT_OF_MEMORY : FVM_ERROR) && !n))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    f.device.failure = 0;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK && n == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pause(f.output, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    output_fixture::consume(f.device, 2);
    f.device.failure = 4;
    if (!(fvm_swf_sound_output_stop_release(f.output) == FVM_ERROR && conversion_fixture::active(f.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.failure = 1;
    if (!(fvm_swf_sound_output_stop_release(f.output) == FVM_ERROR && !conversion_fixture::active(f.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(output_fixture::info(f.output).pending_frames == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int pending = 0;
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.failure = 0;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK && n == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.device.history[8] == f.device.history[2] && f.device.history[12] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(f.output) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pending(f.output, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
