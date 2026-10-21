/* Distinct deep native owners may seek/read/close concurrently after the source VM disappears. */
#include <SDL.h>
#include "../../tests/swf_sound_conversion_fixture.hpp"
#include "flashvm/gc.h"
using namespace conversion_fixture;
struct Job { FVM_SWFSoundVoice *voice; SDL_atomic_t *start; };
static short expected(size_t frame){return (short)((long)((frame*13)%16000)-8000);}
static int SDLCALL read_voice(void *raw)
{
    Job *job=(Job*)raw;while(!SDL_AtomicGet(job->start))SDL_Delay(1);
    short pcm[64];size_t n=0;
    assert(fvm_swf_sound_voice_read(job->voice,1,pcm,64,&n)==FVM_OK && n==1 && pcm[0]==expected(13));
    for(size_t iteration=0;iteration<1000;++iteration)
    {
        size_t at=(iteration*17)%900;
        assert(fvm_swf_sound_voice_seek(job->voice,0,at)==FVM_OK);
        assert(fvm_swf_sound_voice_read(job->voice,64,pcm,64,&n)==FVM_OK && n==64);
        for(size_t i=0;i<n;++i)assert(pcm[i]==expected(at+i));
    }
    fvm_swf_sound_voice_close(job->voice);job->voice=0;return 0;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Samples samples;for(size_t i=0;i<1024;++i)samples.push_back(expected(i));
    Bytes swf=movie(samples,3,1);FVM_SWFSoundVoice *source=0;
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&source)==FVM_OK && fvm_swf_sound_voice_seek(source,0,13)==FVM_OK);
    Job jobs[4];SDL_Thread *threads[4];SDL_atomic_t start;SDL_AtomicSet(&start,0);
    for(unsigned i=0;i<4;++i){jobs[i].voice=0;jobs[i].start=&start;assert(fvm_swf_sound_voice_clone(source,(size_t)-1,&jobs[i].voice)==FVM_OK);}
    assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);fvm_swf_sound_voice_close(source);
    for(unsigned i=0;i<4;++i){threads[i]=SDL_CreateThread(read_voice,"native voice clone",&jobs[i]);assert(threads[i]);}
    SDL_AtomicSet(&start,1);
    for(unsigned i=0;i<4;++i){int status=-1;SDL_WaitThread(threads[i],&status);assert(!status && !jobs[i].voice);}
    return 0;
}
