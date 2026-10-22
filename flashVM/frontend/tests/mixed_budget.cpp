/* A deliberately undersized adapter ceiling rejects mixed preflight before any SDL endpoint. */
#define FVM_DESKTOP_MIXED_FIXTURE_ONLY
#include "mixed_open.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);Bytes swf=mixed_movie();assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_desktop_audio_validate(c)==FVM_OUT_OF_MEMORY);assert(!SDL_WasInit(SDL_INIT_AUDIO));FVM_Object *clip=0;assert(fvm_movieclip_load_swf_ex(c,swf.begin(),swf.size(),"root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_DesktopAudio a;assert(fvm_desktop_audio_open_paused(c,clip,&a,1)==FVM_OUT_OF_MEMORY && !a.device && !a.thread && !a.composition && !a.stop_binding);fvm_desktop_audio_close(&a);assert(!SDL_WasInit(SDL_INIT_AUDIO));fvm_gc_unpin(c,clip);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
