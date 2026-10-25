/* White-box exact epoch bounds reject unrepresentable work before voice/source advancement. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/swf/sound_mix_queue.cpp"
#include "swf_stream_mix_gate_fixture.hpp"


int main()
{
    GateFixture f;
    FVM_SWFStreamConverter cursor = f.converter;
    FVM_SWFStreamOutput wrong = f.output;
    wrong.backend.write = gate_write;
    size_t n = 91;
    if (!(fvm_swf_stream_output_pump_mix_until(&wrong, f.mix, 0, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.mix->clock_submitted = f.mix->clock_played = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 1;
    f.output.info.submitted_frames = f.output.info.played_frames = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 1;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_UNSUPPORTED && !n && !f.fifo.calls &&
          !memcmp(&cursor, &f.converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.mix->clock_submitted = f.mix->clock_played = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 2;
    f.output.info.submitted_frames = f.output.info.played_frames = FVM_SWF_SOUND_OUTPUT_MAX_CLOCK - 2;
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(f.output.info.submitted_frames == FVM_SWF_SOUND_OUTPUT_MAX_CLOCK &&
          f.mix->clock_submitted == FVM_SWF_SOUND_OUTPUT_MAX_CLOCK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 0, &n) == FVM_UNSUPPORTED && !n && f.fifo.total == 2 &&
          !memcmp(&cursor, &f.converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_OK && !f.output.info.submitted_frames &&
          !f.mix->clock_submitted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_mix_queue_set_identity_tracking(f.mix, 0, 0) == FVM_OK &&
          fvm_swf_sound_mix_queue_clock_end(f.mix) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_pump_mix_until(&f.output, f.mix, 1, &n) == FVM_NOT_FOUND && !n &&
          !memcmp(&cursor, &f.converter, sizeof(cursor))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
