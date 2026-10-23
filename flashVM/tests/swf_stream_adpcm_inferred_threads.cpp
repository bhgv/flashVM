/* Four real OS workers independently convert one immutable inferred snapshot after source teardown. */
#include "swf_stream_adpcm_inferred_fixture.hpp"
#include "flashvm/swf_stream_convert.h"
#include "../ports/thread_port.h"
struct Job {const FVM_SWFStreamSoundSnapshot *source;unsigned done;};
static void worker(void *raw)
{
 Job *job=(Job*)raw;
 for(unsigned epoch=0;epoch<32;++epoch){FVM_SWFStreamConverter converter;assert(fvm_swf_stream_converter_init_continuous(&converter,job->source,48000,2)==FVM_OK);size_t total=0;short pcm[62];for(;;){FVM_SWFStreamConvertedChunk chunk;chunk.struct_size=sizeof(chunk);size_t written=0;FVM_Result r=fvm_swf_stream_converter_next(&converter,31,pcm,62,&chunk,&written);if(r==FVM_NOT_FOUND)break;assert(r==FVM_OK&&written==chunk.frames*2);for(size_t f=0;f<chunk.frames;++f){size_t at=(total+f)*44100/48000;long phase=(long)((total+f)*44100%48000);for(unsigned ch=0;ch<2;++ch){long a=(ch?-100:100)+(long)(at%4096);long b=(ch?-100:100)+(long)((at+1<4097?at+1:at)%4096);short expected=(short)((a*(48000-phase)+b*phase)/48000);assert(pcm[f*2+ch]==expected);}}total+=chunk.frames;}assert(total==(4097UL*48000+44100-1)/44100);++job->done;}
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);
 Bits bits=partial_bits(3,2,4097);Bytes swf=swf_block(bits.bytes,2);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_SWFStreamSoundSnapshot *source=0;assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 Job jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].source=source;jobs[i].done=0;threads[i]=fvm_port_thread_start(worker,jobs+i);assert(threads[i]);}for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(jobs[i].done==32);}fvm_swf_stream_sound_snapshot_close(source);return 0;
}
