/* Explicit preparation ceilings and immutable charges are independent of native pointer size. */
#define main adpcm_reference_main
#include "swf_stream_adpcm.cpp"
#undef main
#include <cstring>
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
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
            bits.put(0, 4);
    }
    Bytes swf = swf_block(bits.bytes, 1);
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFStreamSoundSnapshot *raw = 0, *pcm = 0, *bounded = 0;
    assert(fvm_swf_stream_sound_snapshot_open(c, 0, &raw) == FVM_OK);
    assert(fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &pcm) == FVM_OK);
    FVM_SWFStreamSoundStorageInfo a, b;
    a.struct_size = sizeof(a);
    b.struct_size = sizeof(b);
    assert(fvm_swf_stream_sound_snapshot_storage_info(raw, &a) == FVM_OK &&
           a.retained_bytes == a.peak_bytes);
    assert(fvm_swf_stream_sound_snapshot_storage_info(pcm, &b) == FVM_OK);
    assert(b.peak_bytes - b.retained_bytes == bits.bytes.size());
    assert(b.peak_bytes - a.retained_bytes == 4096 * sizeof(short));
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, b.peak_bytes, &bounded) == FVM_OK);
    FVM_SWFStreamSoundStorageInfo exact;
    exact.struct_size = sizeof(exact);
    assert(fvm_swf_stream_sound_snapshot_storage_info(bounded, &exact) == FVM_OK &&
           exact.retained_bytes == b.retained_bytes && exact.peak_bytes == b.peak_bytes);
    fvm_swf_stream_sound_snapshot_close(bounded);
    bounded = reinterpret_cast<FVM_SWFStreamSoundSnapshot *>(1);
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, b.peak_bytes - 1, &bounded) ==
               FVM_OUT_OF_MEMORY &&
           !bounded);
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, 0, &bounded) == FVM_OUT_OF_MEMORY &&
           !bounded);
    size_t old = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 4096;
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, (size_t)-1, &bounded) ==
               FVM_OUT_OF_MEMORY &&
           !bounded);
    rt->config.memory_limit_bytes = old;
    exact.struct_size = 0;
    exact.retained_bytes = 123;
    exact.peak_bytes = 456;
    assert(fvm_swf_stream_sound_snapshot_storage_info(pcm, &exact) == FVM_INVALID_ARGUMENT &&
           exact.retained_bytes == 123 && exact.peak_bytes == 456);
    exact.struct_size = sizeof(exact);
    assert(fvm_swf_stream_sound_snapshot_storage_info(0, &exact) == FVM_INVALID_ARGUMENT &&
           exact.retained_bytes == 123);
    assert(fvm_swf_stream_sound_snapshot_storage_info(pcm, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, b.peak_bytes, 0) ==
           FVM_INVALID_ARGUMENT);
    /* PCM needs no expansion: a successful bounded open has equal retained and peak charges. */
    Bytes tags, head, data;
    head.push_back(0x0e);
    head.push_back(0x3e);
    word(head, 1);
    word(data, 1234);
    tag(tags, 45, head);
    tag(tags, 19, data);
    swf = movie(tags);
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
           fvm_swf_stream_sound_snapshot_open_pcm(c, 0, &bounded) == FVM_OK);
    exact.struct_size = sizeof(exact);
    assert(fvm_swf_stream_sound_snapshot_storage_info(bounded, &exact) == FVM_OK &&
           exact.peak_bytes == exact.retained_bytes);
    size_t cost = exact.peak_bytes;
    fvm_swf_stream_sound_snapshot_close(bounded);
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, cost - 1, &bounded) ==
               FVM_OUT_OF_MEMORY &&
           !bounded);
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c, 0, cost, &bounded) == FVM_OK);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    exact.struct_size = sizeof(exact);
    assert(fvm_swf_stream_sound_snapshot_storage_info(pcm, &exact) == FVM_OK &&
           exact.peak_bytes == b.peak_bytes);
    short output = 0;
    size_t written = 0;
    assert(fvm_swf_stream_sound_snapshot_read_pcm(bounded, 0, 0, 1, &output, 1, &written) ==
               FVM_OK &&
           output == 1234);
    fvm_swf_stream_sound_snapshot_close(raw);
    fvm_swf_stream_sound_snapshot_close(pcm);
    fvm_swf_stream_sound_snapshot_close(bounded);
    return 0;
}
