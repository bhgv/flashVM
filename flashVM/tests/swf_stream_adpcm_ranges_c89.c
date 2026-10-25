/* Strict C89 actual ADPCM calls: authoritative three-frame block and post-VM raw ownership. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_stream_sound.h"

static const unsigned char movie[] = {70, 87, 83, 8, 30, 0,   0, 0,   8,  0, 0, 12, 1, 0, 132,
                                      4,  14, 30, 1, 0,  196, 4, 128, 25, 0, 0, 64, 0, 0, 0};


int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_SWFStreamSoundSnapshot *source = 0;
    size_t written = 91;
    const size_t count = 3;
    short pcm[2] = {123, 456};
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(context, movie, sizeof(movie)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_read_adpcm(context, 0, 0, 3, 1, 2, pcm, 2, &written) == FVM_OK && written == 2 &&
          pcm[0] == 100 && pcm[1] == 100))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_adpcm_counted(context, 0, &count, 1, (size_t)-1, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(source);
    if (!(fvm_swf_stream_sound_snapshot_open(context, 0, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, 3, 0, 2, pcm, 2, &written) == FVM_OK && written == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_read_adpcm(source, 0, 3, 0, 0, 0, 0, &written) == FVM_OK && !written))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(source);
    return 0;
}
