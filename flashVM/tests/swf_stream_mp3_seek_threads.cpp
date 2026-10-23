/* Four OS workers share immutable post-VM MP3 metadata with independent conversion grids. */
#define main original_decode_main
#include "swf_stream_mp3_decode.cpp"
#undef main
#include "flashvm/swf_stream_convert.h"
#include "../ports/thread_port.h"
struct SeekJob { const FVM_SWFStreamSoundSnapshot *source;unsigned rate,channels,done; };
static void seek_worker(void *raw)
{
    SeekJob *job=(SeekJob*)raw;
    FVM_SWFStreamConverter converter;assert(fvm_swf_stream_converter_init(&converter,job->source,job->rate,job->channels)==FVM_OK);
    for(unsigned epoch=0;epoch<64;++epoch)
    {
        FVM_SWFStreamSoundSeekPoint point;point.struct_size=sizeof(point);
        assert(fvm_swf_stream_sound_snapshot_seek_point(job->source,epoch%2?3:1,&point)==FVM_OK);
        assert(fvm_swf_stream_converter_seek(&converter,point.block_index,point.first)==FVM_OK);
        short actual[8],expected[8];size_t written=0,count=0;FVM_SWFStreamConvertedChunk chunk;chunk.struct_size=sizeof(chunk);
        assert(fvm_swf_stream_converter_next(&converter,4,actual,8,&chunk,&written)==FVM_OK && written==4*job->channels);
        assert(fvm_swf_stream_sound_snapshot_read_pcm(job->source,point.block_index,point.first,4,expected,8,&count)==FVM_OK && count==written);
        for(size_t i=0;i<count;++i)assert(actual[i]==expected[i]);
        ++job->done;
    }
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    const mp3_fixture::Fixture &fixture=mp3_fixture::fixtures[0];Bytes swf=stream_movie(fixture,1105);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFStreamSoundSnapshot *source=0;assert(fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    SeekJob jobs[4];FVM_PortThread *threads[4];
    for(unsigned i=0;i<4;++i){jobs[i].source=source;jobs[i].rate=fixture.rate;jobs[i].channels=fixture.channels;jobs[i].done=0;threads[i]=fvm_port_thread_start(seek_worker,jobs+i);assert(threads[i]);}
    for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==64);}
    fvm_swf_stream_sound_snapshot_close(source);return 0;
}
