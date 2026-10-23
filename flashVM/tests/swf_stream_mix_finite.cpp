/* Paired finite EOF tails stop at fourteen event frames instead of padding the last block. */
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 GateFixture f;size_t n=91;
 assert(fvm_swf_stream_output_pump_mix_finite_until(0,f.mix,1,&n)==FVM_INVALID_ARGUMENT && !n);
 for(unsigned step=0;step<20;++step){f.fifo.count=0;FVM_Result r=fvm_swf_stream_output_pump_mix_finite_until(&f.output,f.mix,1,&n);if(r==FVM_NOT_FOUND){assert(!n);break;}assert(r==FVM_OK && n);}
 assert(f.fifo.total==14 && f.output.info.eof && f.output.info.submitted_frames==14);
 const short base[]={30000,-25125,11875,-10250,250,-1875,10500,4125,-30000};
 for(unsigned frame=0;frame<14;++frame)for(unsigned ch=0;ch<2;++ch)assert(f.fifo.samples[frame*2+ch]==(frame<9?base[frame]:-30000));
 FVM_SWFStreamOutputInfo info;info.struct_size=sizeof(info);
 assert(fvm_swf_stream_output_info(&f.output,&info)==FVM_OK && info.played_frames==14);
 FVM_SWFSoundMixQueueIdentityInfo id;id.struct_size=sizeof(id);
 assert(fvm_swf_sound_mix_queue_identity_info(f.mix,8,&id)==FVM_OK && !id.active && id.end_frame==14);
 return 0;
}
