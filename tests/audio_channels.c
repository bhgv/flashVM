/* C89 registration, actual host callback delivery, StopSounds, lifecycle and retry behavior. */
#include "flashvm/multimedia.h"
#include "flashvm/avm1.h"
#include "flashvm/standard.h"
#include "flashvm/execution_mode.h"
#include "flashvm/gc.h"
#include "flashvm/function.h"
#include <assert.h>
#include <string.h>
typedef struct Host {
 FVM_Context *context;FVM_AudioChannel *channel;unsigned writes,stops;
 FVM_Result write_result,stop_result;int probe;
} Host;
static FVM_Result write_pcm(void *raw,const FVM_MediaFrame *frame)
{Host *host=(Host *)raw;assert(frame->samples==2);++host->writes;return host->write_result;}
static FVM_Result stop_pcm(void *raw)
{
 Host *host=(Host *)raw;++host->stops;
 if(host->probe){int active=0;FVM_AudioChannel *out=(FVM_AudioChannel *)1;
 assert(fvm_audio_channel_active(host->channel,&active)==FVM_OK && active);
 assert(fvm_audio_stop_all(host->context)==FVM_INVALID_ARGUMENT);
 assert(fvm_audio_channel_close(host->channel)==FVM_INVALID_ARGUMENT);
 assert(fvm_audio_channel_stop(host->channel)==FVM_INVALID_ARGUMENT);
 assert(fvm_audio_channel_create(host->context,0,&out)==FVM_INVALID_ARGUMENT && !out);
 assert(fvm_gc_collect(host->context,0)==FVM_INVALID_ARGUMENT);
 fvm_context_destroy(host->context); /* Native barrier prevents destruction during callback. */
 }
 return host->stop_result;
}
static void create(FVM_Context *context,Host *host)
{
 FVM_MediaOutput output;memset(host,0,sizeof(*host));host->context=context;
 output.struct_size=sizeof(output);output.userdata=host;output.write=write_pcm;output.stop=stop_pcm;
 assert(fvm_audio_channel_create(context,&output,&host->channel)==FVM_OK);
 memset(&output,0,sizeof(output)); /* Creation must copy the whole callback table. */
}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;unsigned version,mode;
 unsigned char pcm[8]={0};FVM_MediaFrame frame;memset(&frame,0,sizeof(frame));
 frame.kind=FVM_MEDIA_AUDIO;frame.data=pcm;frame.size=sizeof(pcm);frame.channels=2;frame.sample_rate=8000;frame.samples=2;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 assert(fvm_audio_stop_all(0)==FVM_INVALID_ARGUMENT);assert(fvm_audio_channel_close(0)==FVM_OK);
 for(version=1;version<=8;++version)for(mode=0;mode<2;++mode)
 {
 FVM_Context *context=0,*other=0;Host a,b,peer;int active=-1;FVM_Value out;
 unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};const unsigned char stop[]={9,0};
 assert(fvm_context_create(rt,&context)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 swf[3]=(unsigned char)version;assert(fvm_load_memory(context,swf,sizeof(swf))==FVM_OK);
 assert(fvm_standard_install(context)==FVM_OK);
 assert(fvm_execution_set_mode(context,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
 assert(fvm_avm1_execute(context,stop,sizeof(stop))==FVM_OK);
 create(context,&a);create(context,&b);create(other,&peer);
 assert(fvm_audio_channel_active(a.channel,&active)==FVM_OK && !active);
 assert(fvm_audio_channel_stop(a.channel)==FVM_OK && !a.stops);
 frame.size=1;assert(fvm_audio_channel_write(a.channel,&frame)==FVM_INVALID_ARGUMENT && !a.writes);frame.size=sizeof(pcm);
 a.write_result=FVM_ERROR;assert(fvm_audio_channel_write(a.channel,&frame)==FVM_ERROR);
 assert(fvm_audio_channel_active(a.channel,&active)==FVM_OK && !active);a.write_result=FVM_OK;
 assert(fvm_audio_channel_write(a.channel,&frame)==FVM_OK && fvm_audio_channel_write(b.channel,&frame)==FVM_OK);
 assert(fvm_audio_channel_write(peer.channel,&frame)==FVM_OK);
 a.probe=1;a.stop_result=FVM_ERROR;
 assert(fvm_avm1_execute(context,stop,sizeof(stop))==FVM_ERROR);
 assert(a.stops==1 && b.stops==1 && !peer.stops);
 assert(fvm_audio_channel_active(a.channel,&active)==FVM_OK && active);
 assert(fvm_audio_channel_active(b.channel,&active)==FVM_OK && !active);
 assert(fvm_audio_channel_close(a.channel)==FVM_ERROR && a.stops==2);
 a.stop_result=FVM_OK;assert(fvm_avm1_execute(context,stop,sizeof(stop))==FVM_OK && a.stops==3 && b.stops==1);
 assert(fvm_avm1_execute(context,stop,sizeof(stop))==FVM_OK && a.stops==3);
 assert(fvm_audio_channel_write(b.channel,&frame)==FVM_OK);
 assert(fvm_call(context,"stopAllSounds",0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED && b.stops==2);
 {const unsigned char body[]={0x96,5,0,7,42,0,0,0,9,0x3e,0};FVM_Object *function=0;
 assert(fvm_audio_channel_write(a.channel,&frame)==FVM_OK);
 assert(fvm_function_create_avm1(context,body,sizeof(body),0,0,0,&function)==FVM_OK);
 assert(fvm_function_invoke(context,function,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==42 && a.stops==4);}
 assert(fvm_audio_channel_close(a.channel)==FVM_OK);assert(fvm_audio_channel_close(b.channel)==FVM_OK);
 /* Context destruction performs a final stop for still-active registrations. */
 peer.stop_result=FVM_ERROR;fvm_context_destroy(other);assert(peer.stops==1);
 fvm_context_destroy(context);
 }
 fvm_runtime_destroy(rt);return 0;
}
