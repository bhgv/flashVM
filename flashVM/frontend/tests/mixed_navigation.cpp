/* Mixed navigation stages clear before new frame events; pause and retained tail work stay explicit. */
#include "mixed_native.hpp"
static void go(MixedNative &f,unsigned frame)
{
 FVM_Value arg,out;fvm_value_number(&arg,frame);assert(fvm_object_call(f.context,f.clip,"gotoAndPlay",1,&arg,&out)==FVM_OK);
}
int main()
{
 {
 MixedNative f;f.advance();size_t n=0;for(unsigned i=0;i<8;++i){f.fifo.count=0;FVM_Result r=fvm_swf_stream_output_pump_until(&f.audio.stream_output,1,&n);if(r==FVM_NOT_FOUND)break;assert(r==FVM_OK);}assert(f.fifo.total==5);f.fifo.fail_write=1;assert(fvm_test_mixed_refill(&f.audio)==FVM_ERROR);FVM_SWFSoundMixQueuePending tail;tail.struct_size=sizeof(tail);assert(fvm_swf_sound_mix_queue_pending(f.audio.composition,&tail)==FVM_OK && tail.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL);assert(fvm_swf_sound_mix_queue_stop_all(f.audio.composition)==FVM_OK && !active(f.audio.mixer));int pending=0;assert(fvm_desktop_audio_stream_pending(&f.audio,&pending)==FVM_OK && pending);FVM_SWFStreamConverter cursor=f.audio.stream_segments[0].converter;assert(fvm_desktop_audio_pause(&f.audio,1)==FVM_OK && f.fifo.paused);assert(fvm_desktop_audio_stream_pending(&f.audio,&pending)==FVM_OK && !pending);assert(!std::memcmp(&cursor,&f.audio.stream_segments[0].converter,sizeof(cursor)));assert(fvm_desktop_audio_pause(&f.audio,0)==FVM_OK);f.fifo.fail_write=0;f.refill();assert(f.fifo.total==7 && f.fifo.samples[10]==2000 && f.fifo.samples[12]==2000);f.fifo.count=0;assert(fvm_desktop_audio_stream_pending(&f.audio,&pending)==FVM_OK && !pending);
 }
 {
 MixedNative f;f.advance();f.fifo.fail_write=1;assert(fvm_test_mixed_refill(&f.audio)==FVM_ERROR);go(f,1);f.fifo.fail_clear=1;assert(fvm_desktop_audio_timeline(f.context,f.clip,&f.audio)==FVM_ERROR);FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);assert(fvm_swf_sound_mix_queue_pending(f.audio.composition,&pending)==FVM_OK && pending.frames==2);FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);assert(fvm_swf_sound_mix_queue_identity_info(f.audio.composition,7,&id)==FVM_OK && id.end_frame==9);f.fifo.fail_clear=0;assert(fvm_desktop_audio_timeline(f.context,f.clip,&f.audio)==FVM_OK);assert(fvm_swf_sound_mix_queue_pending(f.audio.composition,&pending)==FVM_OK && !pending.frames);f.fifo.fail_write=0;f.advance();assert(f.audio.events==2 && active(f.audio.mixer)==1);assert(fvm_swf_sound_mix_queue_identity_info(f.audio.composition,7,&id)==FVM_OK && id.end_frame==9);f.refill();assert(f.fifo.total==4);for(size_t i=0;i<8;++i)assert(f.fifo.samples[i]==2100);
 }
 return 0;
}
