/* Production C89 worker finite policy ends root events exactly before resuming retained base. */
#define FVM_TEST_MIXED_STOP_FIXTURE_ONLY
#include "mixed_authored_stop.cpp"
int main()
{
 for(unsigned mode=0;mode<2;++mode)
 {
  if(mode&&!fvm_jit_available())continue;
  Bytes swf=stopped_movie();MixedNative f(mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,&swf);f.audio.finite_mixed_tails=1;
  FVM_SWFStreamConverter held=f.audio.stream_segments[0].converter;f.advance();
  for(unsigned i=0;i<8;++i){f.fifo.count=0;f.refill();}
  assert(f.fifo.total==9 && !active(f.audio.mixer) && !memcmp(&held,&f.audio.stream_segments[0].converter,sizeof(held)));
  for(unsigned i=0;i<18;++i)assert(f.fifo.samples[i]==2000);
  assert(fvm_gc_collect(f.context,0)==FVM_OK);FVM_Value out;
  assert(fvm_object_call(f.context,f.clip,"play",0,0,&out)==FVM_OK && fvm_desktop_audio_timeline(f.context,f.clip,&f.audio)==FVM_OK);
  for(unsigned i=0;i<4;++i){f.fifo.count=0;f.refill();}
  assert(f.fifo.total==14);for(unsigned i=18;i<28;++i)assert(f.fifo.samples[i]==100);
  FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(f.context,&stats)==FVM_OK && (mode?stats.native_entries:stats.interpreter_entries)>0);
 }
 return 0;
}
