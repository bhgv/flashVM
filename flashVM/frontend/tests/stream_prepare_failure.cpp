/* Allocation-failure sweep releases every partially prepared PCM header before VM teardown. */
#define main allocation_fixture_main
#include "../../tests/native_instance_allocation.cpp"
#undef main
#define main stream_fixture_main
#include "../../tests/swf_stream_sound.cpp"
#undef main
#include "../common/audio.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
int main(int argc,char **argv)
{
 long begin=0,end=512;bool isolated=argc==3;
 if(isolated){assert(!std::strcmp(argv[1],"--allocation-budget"));char *tail=0;begin=std::strtol(argv[2],&tail,10);assert(tail && !*tail && begin>=0 && begin<512);end=begin+1;}
 else assert(argc==1);
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 Bytes tags,data,empty;data.push_back(0);data.push_back(255);tag(tags,45,header(12));tag(tags,19,data);tag(tags,1,empty);
 tag(tags,45,header(0x3e));tag(tags,1,empty);tag(tags,45,header(0x3b));data.clear();word(data,1000);word(data,2000);tag(tags,19,data);
 Bytes swf=stream_movie(tags,8,3);load(c,swf);assert(fvm_standard_install(c)==FVM_OK);
 FVM_Object *clip=0;assert(fvm_movieclip_load_swf_ex(c,swf.begin(),swf.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
 bool success=false;unsigned failures=0,partial=0;
 for(long budget=begin;budget<end && !success;++budget){FVM_DesktopAudio audio;allowance=budget;FVM_Result result=fvm_desktop_audio_open_paused(c,clip,&audio,1);allowance=-1;
  if(result==FVM_OK){assert(audio.stream_count==3 && audio.stream_source && audio.thread);success=true;}
  else{assert(result==FVM_OUT_OF_MEMORY);++failures;if(audio.stream_source && audio.stream_segments[1].source)++partial;}
  fvm_desktop_audio_close(&audio);assert(!audio.stream_segments && !audio.stream_source && !audio.thread && !audio.device && !audio.stop_binding);
  assert(fvm_gc_step(c,0)==FVM_OK);
 }
 if(!isolated)assert(success && failures && partial);assert(fvm_gc_unpin(c,clip)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);return isolated && !success?77:0;
}
