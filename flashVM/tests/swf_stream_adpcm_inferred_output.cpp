/* Inferred PCM retains exact pending bytes through backpressure, frame gates and staged seek. */
#include "swf_stream_adpcm_inferred_fixture.hpp"
#include "flashvm/swf_stream_output.h"
struct FIFO {size_t queued,total,pending;short witness[6];int reject,fail_clear;};
static FVM_Result send(void *raw,const short *pcm,size_t frames)
{
 FIFO *q=(FIFO*)raw;if(q->pending)assert(q->pending==frames&&!memcmp(q->witness,pcm,frames*2*sizeof(short)));else{q->pending=frames;memcpy(q->witness,pcm,frames*2*sizeof(short));}
 if(q->reject)return FVM_NOT_FOUND;for(size_t i=0;i<frames;++i)assert(pcm[i*2]==100&&pcm[i*2+1]==-100);q->pending=0;q->queued+=frames;q->total+=frames;return FVM_OK;
}
static FVM_Result count(void *raw,size_t *out){*out=((FIFO*)raw)->queued;return FVM_OK;}
static FVM_Result pause_fifo(void *,int){return FVM_OK;}
static FVM_Result clear_fifo(void *raw){FIFO *q=(FIFO*)raw;if(q->fail_clear)return FVM_ERROR;q->queued=q->total=q->pending=0;return FVM_OK;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);
 Bits bits=partial_bits(5,2,17);Bytes swf=swf_block(bits.bytes,2);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_SWFStreamSoundSnapshot *source=0;assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
 FVM_SWFStreamConverter converter;assert(fvm_swf_stream_converter_init_continuous(&converter,source,48000,2)==FVM_OK);FIFO fifo;memset(&fifo,0,sizeof(fifo));FVM_AudioQueueBackend b;memset(&b,0,sizeof(b));b.struct_size=sizeof(b);b.abi_version=FVM_AUDIO_QUEUE_ABI_VERSION;b.sample_rate=48000;b.channels=2;b.userdata=&fifo;b.write=send;b.queued=count;b.pause=pause_fifo;b.clear=clear_fifo;short pcm[6];FVM_SWFStreamOutput output;assert(fvm_swf_stream_output_init(&output,&converter,&b,pcm,6,3,6)==FVM_OK);
 size_t n=91;FVM_SWFStreamConverter held=converter;assert(fvm_swf_stream_output_pump_until(&output,0,&n)==FVM_NOT_FOUND&&!n&&!memcmp(&held,&converter,sizeof(held)));
 fifo.reject=1;assert(fvm_swf_stream_output_pump(&output,&n)==FVM_NOT_FOUND&&!n&&output.info.pending_frames==3);held=converter;fifo.fail_clear=1;assert(fvm_swf_stream_output_seek_frame(&output,1)==FVM_ERROR&&output.info.pending_frames==3&&!memcmp(&held,&converter,sizeof(held)));fifo.fail_clear=0;assert(fvm_swf_stream_output_seek_frame(&output,1)==FVM_OK&&!output.info.submitted_frames&&!output.info.pending_frames);
 for(unsigned k=0;k<16;++k){fifo.queued=0;fifo.reject=1;FVM_Result r=fvm_swf_stream_output_pump(&output,&n);if(output.info.eof){assert(r==FVM_NOT_FOUND);break;}assert(r==FVM_NOT_FOUND&&!n&&output.info.pending_frames);held=converter;short witness[6];memcpy(witness,pcm,sizeof(pcm));fifo.reject=0;assert(fvm_swf_stream_output_pump(&output,&n)==FVM_OK&&n&&!memcmp(&held,&converter,sizeof(held))&&!memcmp(witness,pcm,sizeof(pcm)));}
 fifo.queued=0;FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);assert(fvm_swf_stream_output_info(&output,&info)==FVM_OK&&info.eof&&info.submitted_frames==19&&info.played_frames==19&&!info.queued_frames&&fifo.total==19);fvm_swf_stream_sound_snapshot_close(source);return 0;
}
