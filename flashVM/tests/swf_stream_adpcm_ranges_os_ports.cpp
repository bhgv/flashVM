/* Four real native workers decode disjoint ranges from shared encoded ADPCM after VM teardown. */
#define main adpcm_reference_main
#include "swf_stream_adpcm.cpp"
#undef main
#include "../ports/thread_port.h"
#define RANGE_WORKERS 4U
#define RANGE_PASSES 200U
#define RANGE_FRAMES 4099U
struct RangeJob
{
    const FVM_SWFStreamSoundSnapshot *source;
    unsigned worker;
    int failed;
    size_t decoded;
};
/* Only immutable source data is shared; every output and counter belongs to one joined worker. */
static void range_worker(void *raw)
{
    RangeJob *job = static_cast<RangeJob *>(raw);
    for (unsigned pass = 0; pass < RANGE_PASSES; ++pass)
    {
        size_t first = (job->worker * 37 + pass * 41) % RANGE_FRAMES;
        size_t frames = RANGE_FRAMES - first;
        if (frames > 11) frames = 11;
        short pcm[22];
        size_t written = 91;
        if (fvm_swf_stream_sound_snapshot_read_adpcm(job->source, 0, RANGE_FRAMES,
                                                     first, frames, pcm, 22, &written) != FVM_OK ||
            written != frames * 2)
        {
            job->failed = 1;
            return;
        }
        for (size_t f = 0; f < frames; ++f)
            for (unsigned ch = 0; ch < 2; ++ch)
                if (pcm[f * 2 + ch] != (ch ? -100L : 100L) + (long)((first + f) % 4096) * 3)
                {
                    job->failed = 1;
                    return;
                }
        job->decoded += frames;
    }
}
int main()
{
    Bits bits;
    bits.put(0, 2);
    for (unsigned f = 0; f < RANGE_FRAMES; ++f)
        for (unsigned ch = 0; ch < 2; ++ch)
        {
            if (!(f % 4096))
            {
                bits.put(ch ? 65436 : 100, 16);
                bits.put(0, 6);
            }
            else bits.put(0, 2);
        }
    Bytes swf = swf_block(bits.bytes, 2);
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_SWFStreamSoundSnapshot *source = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
           fvm_context_create(runtime, &context) == FVM_OK &&
           fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK &&
           fvm_swf_stream_sound_snapshot_open(context, 0, &source) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    RangeJob jobs[RANGE_WORKERS];
    FVM_PortThread *threads[RANGE_WORKERS];
    for (unsigned i = 0; i < RANGE_WORKERS; ++i)
    {
        jobs[i].source = source; jobs[i].worker = i; jobs[i].failed = 0; jobs[i].decoded = 0;
        threads[i] = fvm_port_thread_start(range_worker, &jobs[i]);
        assert(threads[i]);
    }
    for (unsigned i = 0; i < RANGE_WORKERS; ++i)
    {
        fvm_port_thread_join(threads[i]);
        assert(!jobs[i].failed && jobs[i].decoded > 0);
    }
    fvm_swf_stream_sound_snapshot_close(source);
    return 0;
}
