/* Render allocation failures leave native output/binding untouched; limits cover both PCM buffers. */
#define main event_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"
using namespace sound_fixture;
static unsigned writes;
static FVM_Result write(void*,const FVM_MediaFrame *f){++writes;assert(f->samples==4 && f->size==8);return FVM_OK;}
static FVM_Result stop(void*){return FVM_OK;}
int main(){FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);const unsigned char data[]={232,3,24,252};Bytes defs;tag(defs,14,sound(7,0x3e,2,data,4));Bytes e;word(e,7);e.push_back(4);word(e,2);tag(defs,15,e);Bytes swf=movie(defs);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_MediaOutput out={sizeof(out),0,write,stop};FVM_AudioChannel *channel=0;assert(fvm_audio_channel_create(c,&out,&channel)==FVM_OK);bool failed=false,passed=false;for(long boundary=0;boundary<16 && !passed;++boundary){allowance=boundary;FVM_Result r=fvm_swf_sound_event_submit(c,0,channel);allowance=-1;if(r==FVM_OUT_OF_MEMORY){failed=true;int active=99;assert(!writes && fvm_audio_channel_active(channel,&active)==FVM_OK && !active);}else{assert(r==FVM_OK && writes==1);passed=true;}}assert(failed && passed);assert(fvm_audio_channel_stop(channel)==FVM_OK);unsigned long limit=rt->config.memory_limit_bytes;rt->config.memory_limit_bytes=11;assert(fvm_swf_sound_event_submit(c,0,channel)==FVM_OUT_OF_MEMORY && writes==1);rt->config.memory_limit_bytes=12;assert(fvm_swf_sound_event_submit(c,0,channel)==FVM_OK && writes==2);rt->config.memory_limit_bytes=limit;allowance=0;assert(fvm_audio_channel_stop(channel)==FVM_OK && fvm_audio_channel_close(channel)==FVM_OK);allowance=-1;fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;}
