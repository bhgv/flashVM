/* Streaming inspection and PCM extraction allocate nothing and reject terminal VM phases. */
#define main stream_allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main stream_reference_main
#include "swf_stream_sound.cpp"

#undef main


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
    Bytes tags, pcm;
    pcm.push_back(255);
    tag(tags, 45, header(12));
    tag(tags, 19, pcm);
    load(c, movie(tags));
    FVM_SWFStreamSoundInfo info;
    info.struct_size = sizeof(info);
    FVM_SWFStreamSoundBlock block;
    block.struct_size = sizeof(block);
    size_t n = 0, count = 0;
    short out = 0;
    allowance = 0;
    if (!(fvm_swf_stream_sound_count(c, &count) == FVM_OK && count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_at(c, 0, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_block_count(c, 0, &count) == FVM_OK && count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_block_at(c, 0, 0, &block) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 1, &out, 1, &n) == FVM_OK && out == 32512 && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = true;
    if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    c->gc_releasing = true;
    if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_releasing = false;
    c->destroying = true;
    if (!(fvm_swf_stream_sound_count(c, &count) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->destroying = false;
    allowance = -1;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 1, &out, 1, &n) == FVM_OK && out == 32512))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes empty;
    load(c, movie(empty));
    n = 99;
    if (!(fvm_swf_stream_sound_read_pcm(c, 0, 0, 0, 1, &out, 1, &n) == FVM_NOT_FOUND && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
