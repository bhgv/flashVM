/* Opposite event voices cancel before saturation in the actual desktop worker after VM teardown. */
#include "mixed_native.hpp"
static Bytes clipping_movie()
{
 Bytes tags,head,pcm;Samples positive,negative;for(unsigned i=0;i<8;++i)positive.push_back(30000);for(unsigned i=0;i<12;++i)negative.push_back(-30000);definition(tags,7,3,1,positive);definition(tags,8,3,1,negative);head.push_back(0x0e);head.push_back(0x3e);sound_fixture::word(head,8);sound_fixture::tag(tags,45,head);const short base[]={30000,-30000,20000,-20000,10000,-10000,30000,-30000};for(unsigned i=0;i<8;++i)sound_fixture::word(pcm,(unsigned short)base[i]);sound_fixture::tag(tags,19,pcm);event(tags,7);event(tags,8);return sound_fixture::movie(tags);
}
int main()
{
 Bytes swf=clipping_movie();MixedNative f(FVM_EXECUTION_INTERPRETER,&swf);f.advance();assert(f.audio.events==2);assert(fvm_movieclip_set_sound_callback(f.context,0,0)==FVM_OK && fvm_audio_stop_binding_close(f.audio.stop_binding)==FVM_OK && fvm_gc_unpin(f.context,f.clip)==FVM_OK);f.audio.stop_binding=0;fvm_context_destroy(f.context);fvm_runtime_destroy(f.rt);f.context=0;f.rt=0;f.clip=0;for(unsigned i=0;i<16;++i){f.fifo.count=0;f.refill();}const short expected[]={30000,-25125,11875,-10250,250,-1875,10500,4125,-30000,-30000,-30000,-30000,-30000,-30000,0};assert(f.fifo.total==15);for(unsigned i=0;i<15;++i)assert(f.fifo.samples[i*2]==expected[i] && f.fifo.samples[i*2+1]==expected[i]);return 0;
}
