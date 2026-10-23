/* Actual desktop refill mixes base once, retries bytes and drains longer event voices after EOF. */
#include "mixed_native.hpp"
int main()
{
 for(unsigned tracked=0;tracked<2;++tracked)
 {
 MixedNative f;if(tracked){size_t bytes=0;assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,1,1,&bytes)==FVM_OK&&fvm_swf_sound_mix_queue_set_origin_tracking(f.audio.composition,1,bytes)==FVM_OK);}
 FVM_SWFStreamMixInfo paired;memset(&paired,0,sizeof(paired));paired.struct_size=sizeof(paired);paired.stream.submitted_frames=91;assert(fvm_desktop_audio_mix_info(&f.audio,&paired)==(tracked?FVM_OK:FVM_UNSUPPORTED));assert(paired.stream.submitted_frames==(tracked?0:91));f.advance();f.fifo.fail_write=1;assert(fvm_test_mixed_refill(&f.audio)==FVM_ERROR && !f.fifo.total);FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);assert(fvm_swf_sound_mix_queue_pending(f.audio.composition,&pending)==FVM_OK && pending.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE && pending.frames==2);f.fifo.fail_write=0;
 /* The final two-frame block contains one event frame and one geometry-dependent padding frame. */
 for(unsigned i=0;i<16;++i){f.fifo.count=0;f.refill();}assert(f.fifo.total==10);for(size_t i=0;i<10;++i){short expected=i<5?2100:i<9?2000:0;assert(f.fifo.samples[i*2]==expected && f.fifo.samples[i*2+1]==expected);}f.fifo.count=0;FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);assert(fvm_desktop_audio_stream_info(&f.audio,&info)==FVM_OK && info.eof && info.submitted_frames==10 && info.played_frames==10);int has=91;assert(fvm_desktop_audio_stream_pending(&f.audio,&has)==FVM_OK && !has);if(tracked){assert(fvm_desktop_audio_mix_info(&f.audio,&paired)==FVM_OK&&paired.stream.submitted_frames==10&&paired.stream.played_frames==10&&paired.origins.base_submitted_frames==5&&paired.origins.tail_submitted_frames==5);} }return 0;
}
