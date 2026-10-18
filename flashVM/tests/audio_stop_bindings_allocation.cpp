/* Registration OOM/capacity rollback; stop/detach/terminal cleanup allocate nothing. */
#define main allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "flashvm/audio_stop.h"
#include "flashvm/multimedia.h"
static FVM_Result stop(void *raw) { ++*(unsigned *)raw; return FVM_OK; }
int main()
{
 FVM_Config cfg;FVM_Runtime *r=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&r)==FVM_OK);
 bool failed=false,passed=false;unsigned calls=0;
 for(long boundary=0;boundary<10 && !passed;++boundary)
 {
  FVM_Context *c=0;assert(fvm_context_create(r,&c)==FVM_OK);
  FVM_AudioStopBinding *binding=(FVM_AudioStopBinding *)1;
  allowance=boundary;FVM_Result result=fvm_audio_stop_binding_create(c,stop,&calls,&binding);allowance=-1;
  if(result==FVM_OUT_OF_MEMORY){failed=true;assert(!binding && c->audio_stop_bindings.empty());}
  else{assert(result==FVM_OK && binding);passed=true;allowance=0;assert(fvm_audio_stop_all(c)==FVM_OK && fvm_audio_stop_binding_close(binding)==FVM_OK);allowance=-1;}
  fvm_context_destroy(c);
 }
 assert(failed && passed && calls==1);
 FVM_Context *c=0;assert(fvm_context_create(r,&c)==FVM_OK);
 FVM_AudioStopBinding *bindings[FVM_AUDIO_STOP_MAX_BINDINGS];
 for(unsigned i=0;i<FVM_AUDIO_STOP_MAX_BINDINGS;++i)assert(fvm_audio_stop_binding_create(c,stop,&calls,&bindings[i])==FVM_OK);
 FVM_AudioStopBinding *extra=(FVM_AudioStopBinding *)1;
 assert(fvm_audio_stop_binding_create(c,stop,&calls,&extra)==FVM_OUT_OF_MEMORY && !extra);
 allowance=0;assert(fvm_audio_stop_all(c)==FVM_OK);
 for(unsigned i=0;i<FVM_AUDIO_STOP_MAX_BINDINGS;++i)assert(fvm_audio_stop_binding_close(bindings[i])==FVM_OK);
 fvm_context_destroy(c);allowance=-1;
 assert(calls==1+FVM_AUDIO_STOP_MAX_BINDINGS);fvm_runtime_destroy(r);return 0;
}
