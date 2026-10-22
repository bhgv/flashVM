/* Actual desktop refill mixes base once, retries bytes and drains longer event voices after EOF. */
#include "mixed_native.hpp"
int main()
{
 MixedNative f;f.advance();f.fifo.fail_write=1;assert(fvm_test_mixed_refill(&f.audio)==FVM_ERROR && !f.fifo.total);FVM_SWFSoundMixQueuePending pending;pending.struct_size=sizeof(pending);assert(fvm_swf_sound_mix_queue_pending(f.audio.composition,&pending)==FVM_OK && pending.origin==FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE && pending.frames==2);f.fifo.fail_write=0;
 for(unsigned i=0;i<16;++i){f.fifo.count=0;f.refill();}assert(f.fifo.total==9);for(size_t i=0;i<9;++i){short expected=i<5?2100:2000;assert(f.fifo.samples[i*2]==expected && f.fifo.samples[i*2+1]==expected);}f.fifo.count=0;FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);assert(fvm_desktop_audio_stream_info(&f.audio,&info)==FVM_OK && info.eof && info.submitted_frames==9 && info.played_frames==9);int has=91;assert(fvm_desktop_audio_stream_pending(&f.audio,&has)==FVM_OK && !has);return 0;
}
