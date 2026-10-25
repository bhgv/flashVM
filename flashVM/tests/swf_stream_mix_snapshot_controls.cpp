/* Failed paired queries preserve outputs and stream cache, including callback exceptions/reentry. */
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mix_origin_fixture.hpp"


static void rejected(GateFixture &f, FVM_Result expected)
{
    FVM_SWFStreamMixInfo info;
    memset(&info, 0x35, sizeof(info));
    info.struct_size = sizeof(info);
    FVM_SWFStreamMixInfo before = info;
    FVM_SWFStreamOutputInfo cached = f.output.info;
    if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &info) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(&info, &before, sizeof(info)) && !memcmp(&cached, &f.output.info, sizeof(cached))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv)
{
    bool exceptions = argc == 1;
    if (!(argc == 1 || (argc == 2 && !strcmp(argv[1], "--no-throw"))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GateFixture f;
    rejected(f, FVM_UNSUPPORTED);
    enable_origins(f);
    size_t n = 0;
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 0, &n) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_count = 1;
    rejected(f, FVM_ERROR);
    f.fifo.fail_count = 0;
    if (exceptions)
    {
        f.fifo.throw_count = 1;
        rejected(f, FVM_ERROR);
        f.fifo.throw_count = 0;
    }
    f.fifo.count = 3;
    rejected(f, FVM_ERROR);
    f.fifo.count = 2;
    f.fifo.reenter = 1;
    FVM_SWFStreamMixInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.reenter = 0;
    GateFixture other;
    size_t calls = f.fifo.count_calls;
    if (!(fvm_swf_stream_output_mix_info(&f.output, other.mix, &info) == FVM_INVALID_ARGUMENT &&
          calls == f.fifo.count_calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.output.info.submitted_frames = 1;
    rejected(f, FVM_INVALID_ARGUMENT);
    if (!(calls == f.fifo.count_calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.output.info.submitted_frames = 2;
    info.struct_size = 0;
    if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_write = 2;
    if (!(fvm_swf_stream_output_pump_mix_finite_until(&f.output, f.mix, 1, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamOutputInfo cache = f.output.info;
    FVM_SWFStreamConverter cursor = f.converter;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &info) == FVM_OK && info.stream.pending_frames == 2 &&
          info.stream.submitted_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(&cursor, &f.converter, sizeof(cursor)) && cache.chunk.frames == info.stream.chunk.frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 1;
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &info) == FVM_OK && info.stream.pending_frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    f.fifo.fail_clear = 0;
    if (!(fvm_swf_stream_output_seek(&f.output, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_output_mix_info(&f.output, f.mix, &info) == FVM_OK && !info.stream.pending_frames &&
          !info.stream.submitted_frames && !info.origins.tail_submitted_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
