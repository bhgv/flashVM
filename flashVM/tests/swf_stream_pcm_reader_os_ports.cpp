/* Independent native worker cursors share immutable stream bytes after VM/runtime teardown. */
#define main stream_fixture_main
#include "swf_stream_sound.cpp"
#undef main
#include "flashvm/swf_stream_pcm.h"
#include "../ports/thread_port.h"
#define READER_WORKERS 4U
#define READER_PASSES 500U
struct ReaderJob { const FVM_SWFStreamSoundSnapshot *snapshot; int failed; size_t samples; };
/* Each worker owns every mutable field; source vectors are only read through const APIs. */
static void read_worker(void *raw)
{
    ReaderJob *job=(ReaderJob *)raw;
    FVM_SWFStreamPCMReader reader;
    if(fvm_swf_stream_pcm_reader_init(&reader,job->snapshot)!=FVM_OK) { job->failed=1;return; }
    const short expected[]={-32768,32512,0};
    for(unsigned pass=0;pass<READER_PASSES;++pass) {
        if(fvm_swf_stream_pcm_reader_seek(&reader,0,0)!=FVM_OK) { job->failed=1;return; }
        size_t count=0;
        for(;;) {
            short pcm=42;size_t written=999;
            FVM_SWFStreamPCMChunk chunk;chunk.struct_size=sizeof(chunk);
            FVM_Result result=fvm_swf_stream_pcm_reader_next(&reader,1,&pcm,1,&chunk,&written);
            if(result==FVM_NOT_FOUND)break;
            if(result!=FVM_OK || written!=1 || count>=3 || pcm!=expected[count] || chunk.frame!=1) { job->failed=1;return; }
            ++count;++job->samples;
        }
        if(count!=3) { job->failed=1;return; }
    }
}
/* Real OS threads join before the single owning host closes the immutable source. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *context=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&context)==FVM_OK);
    Bytes tags,pcm;pcm.push_back(0);pcm.push_back(255);pcm.push_back(128);
    tag(tags,45,header(12));tag(tags,19,pcm);load(context,movie(tags));
    FVM_SWFStreamSoundSnapshot *snapshot=0;
    assert(fvm_swf_stream_sound_snapshot_open(context,0,&snapshot)==FVM_OK);
    fvm_context_destroy(context);fvm_runtime_destroy(runtime);
    ReaderJob jobs[READER_WORKERS];FVM_PortThread *threads[READER_WORKERS];
    for(unsigned i=0;i<READER_WORKERS;++i) {
        jobs[i].snapshot=snapshot;jobs[i].failed=0;jobs[i].samples=0;
        threads[i]=fvm_port_thread_start(read_worker,jobs+i);assert(threads[i]);
    }
    for(unsigned i=0;i<READER_WORKERS;++i) {
        fvm_port_thread_join(threads[i]);assert(!jobs[i].failed && jobs[i].samples==3*READER_PASSES);
    }
    fvm_swf_stream_sound_snapshot_close(snapshot);
    return 0;
}
