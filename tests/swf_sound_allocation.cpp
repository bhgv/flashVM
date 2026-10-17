/* Lookup/read are allocation-free; native PCM submission OOM cannot activate or replay output. */
#define main sound_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"
using namespace sound_fixture;
static unsigned writes,stops;
static FVM_Result write_pcm(void *,const FVM_MediaFrame *frame){assert(frame->samples==2 && frame->channels==2 && frame->size==8);++writes;return FVM_OK;}
static FVM_Result stop_pcm(void *){++stops;return FVM_OK;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 const unsigned char data[]={1,0,255,255,2,0,254,255};Bytes definitions;tag(definitions,14,sound(7,0x3f,2,data,sizeof(data)));Bytes swf=movie(definitions);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 FVM_MediaOutput output;output.struct_size=sizeof(output);output.userdata=0;output.write=write_pcm;output.stop=stop_pcm;FVM_AudioChannel *channel=0;assert(fvm_audio_channel_create(c,&output,&channel)==FVM_OK);
 short pcm[4]={0};size_t count=0;FVM_SWFSoundInfo info;info.struct_size=sizeof(info);allowance=0;
 assert(fvm_swf_sound_count(c,&count)==FVM_OK && count==1 && fvm_swf_sound_at(c,0,&info)==FVM_OK && fvm_swf_sound_info(c,7,&info)==FVM_OK && fvm_swf_sound_read_pcm(c,7,0,2,pcm,4,&count)==FVM_OK && count==4);allowance=-1;
 bool failed=false,succeeded=false;
 for(long boundary=0;boundary<16 && !succeeded;++boundary){unsigned before=writes;allowance=boundary;FVM_Result result=fvm_swf_sound_submit(c,7,channel,0,2);allowance=-1;
 if(result==FVM_OUT_OF_MEMORY){failed=true;int active=1;assert(writes==before && fvm_audio_channel_active(channel,&active)==FVM_OK && !active);}else{assert(result==FVM_OK && writes==before+1);succeeded=true;}}
 assert(failed && succeeded);unsigned long old=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=1;assert(fvm_swf_sound_submit(c,7,channel,0,2)==FVM_OUT_OF_MEMORY && writes==1);rt->config.memory_limit_bytes=old;
 allowance=0;assert(fvm_audio_stop_all(c)==FVM_OK && fvm_audio_channel_close(channel)==FVM_OK);allowance=-1;assert(stops==1);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
