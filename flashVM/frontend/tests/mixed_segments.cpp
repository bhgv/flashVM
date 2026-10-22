/* Root source transitions preserve mixed clocks; future frame gates hold voices until base resumes. */
#include "mixed_native.hpp"
static Bytes segment_movie()
{
 Bytes tags,head,pcm,second,empty;Samples samples;for(unsigned i=0;i<8;++i)samples.push_back(2000);definition(tags,7,3,1,samples);head.push_back(0x0e);head.push_back(0x3e);sound_fixture::word(head,4);sound_fixture::tag(tags,45,head);for(unsigned i=0;i<4;++i){sound_fixture::word(pcm,100);sound_fixture::word(second,300);}sound_fixture::tag(tags,19,pcm);event(tags,7);sound_fixture::tag(tags,1,empty);sound_fixture::tag(tags,1,empty);sound_fixture::tag(tags,45,head);sound_fixture::tag(tags,19,second);Bytes swf=sound_fixture::movie(tags);swf[12]=3;return swf;
}
int main()
{
 Bytes swf=segment_movie();MixedNative f(FVM_EXECUTION_INTERPRETER,&swf);for(unsigned epoch=0;epoch<2;++epoch){if(epoch){FVM_Value arg,out;fvm_value_number(&arg,1);assert(fvm_object_call(f.context,f.clip,"gotoAndPlay",1,&arg,&out)==FVM_OK && fvm_desktop_audio_timeline(f.context,f.clip,&f.audio)==FVM_OK);}f.advance();for(unsigned i=0;i<4;++i){f.fifo.count=0;f.refill();}assert(f.fifo.total==epoch*10+5 && f.audio.stream_index==1 && active(f.audio.mixer)==1);f.advance();f.refill();assert(f.fifo.total==epoch*10+5);f.advance();for(unsigned i=0;i<4;++i){f.fifo.count=0;f.refill();}assert(f.fifo.total==(epoch+1)*10);for(unsigned i=0;i<10;++i){short expected=i<5?2100:i<9?2300:300;assert(f.fifo.samples[(epoch*10+i)*2]==expected && f.fifo.samples[(epoch*10+i)*2+1]==expected);}FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);f.fifo.count=0;assert(fvm_desktop_audio_stream_info(&f.audio,&info)==FVM_OK && info.submitted_frames==10 && info.played_frames==10);}
 return 0;
}
