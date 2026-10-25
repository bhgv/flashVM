/* Continuous boundary reads allocate nothing after VM teardown, including empty-block lookahead. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main stream_reference_main
#include "swf_stream_sound.cpp"

#undef main
#include "flashvm/swf_stream_convert.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes tags, empty, left, right;
    tag(tags, 45, header(0x32));
    word(left, 0);
    word(right, 24000);
    tag(tags, 19, left);
    tag(tags, 1, empty);
    for (unsigned i = 0; i < 32; ++i)
    {
        tag(tags, 19, empty);
        tag(tags, 1, empty);
    }
    tag(tags, 19, right);
    load(c, stream_movie(tags, 8, 34));
    FVM_SWFStreamSoundSnapshot *snapshot = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &snapshot) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    allowance = 0;
    FVM_SWFStreamConverter state, saved;
    if (!(fvm_swf_stream_converter_init_continuous(&state, snapshot, 11024, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamConvertedChunk chunk;
    chunk.struct_size = sizeof(chunk);
    short pcm[4];
    size_t written = 999;
    if (!(fvm_swf_stream_converter_next(&state, 2, pcm, 4, &chunk, &written) == FVM_OK && written == 2 && pcm[0] == 0 &&
          pcm[1] == 12000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 32; ++i)
    {
        if (!(fvm_swf_stream_converter_next(&state, 2, pcm, 4, &chunk, &written) == FVM_OK && written == 0 &&
              (chunk.flags & FVM_STREAM_CONVERT_EMPTY)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_stream_converter_next(&state, 2, pcm, 4, &chunk, &written) == FVM_OK && written == 2 &&
          pcm[0] == 24000 && pcm[1] == 24000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_seek(&state, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    saved = state;
    pcm[0] = 91;
    written = 999;
    if (!(fvm_swf_stream_converter_next(&state, 2, pcm, 1, &chunk, &written) == FVM_INVALID_ARGUMENT && pcm[0] == 91 &&
          written == 999 && !std::memcmp(&saved, &state, sizeof(state))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_seek(&state, 33, 0) == FVM_OK && state.continuous))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_converter_next(&state, 2, pcm, 4, &chunk, &written) == FVM_OK && pcm[0] == 24000 &&
          pcm[1] == 24000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(snapshot);
    allowance = -1;
    return 0;
}
