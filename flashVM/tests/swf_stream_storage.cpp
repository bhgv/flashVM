/* Explicit preparation ceilings and immutable charges are independent of native pointer size. */
#define main adpcm_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_stream_adpcm.cpp"

#undef main
#include <cstring>


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
    Bits bits;
    bits.put(2, 2);
    for (unsigned f = 0; f < 4096; ++f)
    {
        if (!f)
        {
            bits.put(100, 16);
            bits.put(0, 6);
        }
        else
        {
            bits.put(0, 4);
        }
    }
    Bytes swf = swf_block(bits.bytes, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundSnapshot *raw = 0, *pcm = 0, *bounded = 0;
    if (!(fvm_swf_stream_sound_snapshot_open(c, 0, &raw) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &pcm) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundStorageInfo a, b;
    a.struct_size = sizeof(a);
    b.struct_size = sizeof(b);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(raw, &a) == FVM_OK && a.retained_bytes == a.peak_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_storage_info(pcm, &b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(b.peak_bytes - b.retained_bytes == bits.bytes.size()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(b.peak_bytes - a.retained_bytes == 4096 * sizeof(short)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Encoded and normalized budgets charge different ownership, with no codec conversion. */
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, a.peak_bytes - 1, &bounded) == FVM_OUT_OF_MEMORY &&
          !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, 0, &bounded) == FVM_OUT_OF_MEMORY && !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, a.peak_bytes, &bounded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundBlock encoded;
    encoded.struct_size = sizeof(encoded);
    if (!(fvm_swf_stream_sound_snapshot_block_at(bounded, 0, &encoded) == FVM_OK &&
          encoded.data_size == bits.bytes.size() && !std::memcmp(encoded.data, bits.bytes.begin(), bits.bytes.size())))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(bounded);
    bounded = 0;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, b.peak_bytes, &bounded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFStreamSoundStorageInfo exact;
    exact.struct_size = sizeof(exact);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(bounded, &exact) == FVM_OK &&
          exact.retained_bytes == b.retained_bytes && exact.peak_bytes == b.peak_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(bounded);
    bounded = reinterpret_cast<FVM_SWFStreamSoundSnapshot *>(1);
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, b.peak_bytes - 1, &bounded) == FVM_OUT_OF_MEMORY &&
          !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, 0, &bounded) == FVM_OUT_OF_MEMORY && !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t old = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 4096;
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, (size_t)-1, &bounded) == FVM_OUT_OF_MEMORY && !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, (size_t)-1, &bounded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(bounded);
    rt->config.memory_limit_bytes = 1;
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, (size_t)-1, &bounded) == FVM_OUT_OF_MEMORY && !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = old;
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(0, 0, 0, &bounded) == FVM_INVALID_ARGUMENT && !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_bounded(c, 0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    exact.struct_size = 0;
    exact.retained_bytes = 123;
    exact.peak_bytes = 456;
    if (!(fvm_swf_stream_sound_snapshot_storage_info(pcm, &exact) == FVM_INVALID_ARGUMENT &&
          exact.retained_bytes == 123 && exact.peak_bytes == 456))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    exact.struct_size = sizeof(exact);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(0, &exact) == FVM_INVALID_ARGUMENT && exact.retained_bytes == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_storage_info(pcm, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, b.peak_bytes, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* PCM needs no expansion: a successful bounded open has equal retained and peak charges. */
    Bytes tags, head, data;
    head.push_back(0x0e);
    head.push_back(0x3e);
    word(head, 1);
    word(data, 1234);
    tag(tags, 45, head);
    tag(tags, 19, data);
    swf = movie(tags);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
          fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &bounded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    exact.struct_size = sizeof(exact);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(bounded, &exact) == FVM_OK &&
          exact.peak_bytes == exact.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t cost = exact.peak_bytes;
    fvm_swf_stream_sound_snapshot_close(bounded);
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, cost - 1, &bounded) == FVM_OUT_OF_MEMORY && !bounded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, cost, &bounded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    exact.struct_size = sizeof(exact);
    if (!(fvm_swf_stream_sound_snapshot_storage_info(pcm, &exact) == FVM_OK && exact.peak_bytes == b.peak_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short output = 0;
    size_t written = 0;
    if (!(fvm_swf_stream_sound_snapshot_read_pcm(bounded, 0, 0, 1, &output, 1, &written) == FVM_OK && output == 1234))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_stream_sound_snapshot_close(raw);
    fvm_swf_stream_sound_snapshot_close(pcm);
    fvm_swf_stream_sound_snapshot_close(bounded);
    return 0;
}
