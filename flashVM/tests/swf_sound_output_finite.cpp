/* Finite output emits exact converted duration while legacy priming remains explicit. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_output_finite_fixture.hpp"


int main()
{
    FiniteOutput f;
    size_t n = 91;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_NOT_FOUND && !n && !f.device.values))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.start();
    for (unsigned i = 0; i < 3; ++i)
    {
        f.device.queued = 0;
        if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_OK && n == (i < 2 ? 4 : 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(f.device.values == 18 && !conversion_fixture::active(f.mixer)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.device.queued = 0;
    if (!(fvm_swf_sound_output_pump_finite(f.output, &n) == FVM_NOT_FOUND && !n && f.device.values == 18))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    conversion_fixture::Samples samples;
    for (unsigned i = 0; i < 8; ++i)
    {
        samples.push_back((short)(2000 + i * 17));
    }
    for (unsigned frame = 0; frame < 9; ++frame)
    {
        for (unsigned ch = 0; ch < 2; ++ch)
        {
            if (!(f.device.history[frame * 2 + ch] ==
                  conversion_fixture::oracle(samples, 44100, 1, 48000, 2, frame, ch)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(fvm_swf_sound_output_pump(f.output, &n) == FVM_OK && n == 4 && f.device.values == 26))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 18; i < 26; ++i)
    {
        if (!(!f.device.history[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
