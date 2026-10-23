/* Enumerate candidate source counts independently for all code geometries and restart boundaries. */
#include "swf_stream_adpcm_inferred_fixture.hpp"
int main()
{
 const size_t counts[]={0,1,2,3,7,17,4094,4095,4096,4097,8191,8192,8193};unsigned accepted=0,ambiguous=0;
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);
 for(unsigned width=2;width<=5;++width)for(unsigned channels=1;channels<=2;++channels)for(unsigned k=0;k<sizeof(counts)/sizeof(counts[0]);++k)
 {
  size_t frames=counts[k];Bits bits=partial_bits(width,channels,frames);Bytes swf=swf_block(bits.bytes,channels);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
  FVM_SWFStreamSoundSnapshot *legacy=0,*source=0;FVM_Result old=fvm_swf_stream_sound_snapshot_open_pcm_bounded(c,0,(size_t)-1,&legacy);size_t expected=0,candidates=0;
  if(old==FVM_OK){FVM_SWFStreamSoundBlock b;b.struct_size=sizeof(b);assert(fvm_swf_stream_sound_snapshot_block_at(legacy,0,&b)==FVM_OK);expected=b.sample_count;fvm_swf_stream_sound_snapshot_close(legacy);}
  else
  {
   assert(old==FVM_UNSUPPORTED);
   for(size_t n=frames>8?frames-8:0;n<=frames+8;++n){size_t whole=n/4096,tail=n%4096;size_t length=2+whole*channels*(22+4095*width)+(tail?channels*(22+(tail-1)*width):0);if((length+7)/8!=bits.bytes.size())continue;unsigned pad=(unsigned)(bits.bytes.size()*8-length);if(pad&&(bits.bytes[bits.bytes.size()-1]&((1U<<pad)-1)))continue;++candidates;expected=n;}
  }
  FVM_Result result=fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source);
  if(old==FVM_OK||candidates==1){assert(result==FVM_OK);verify_partial(source,expected,channels,width);fvm_swf_stream_sound_snapshot_close(source);++accepted;}
  else{assert(candidates>1&&result==FVM_UNSUPPORTED&&!source);++ambiguous;}
 }
 assert(accepted&&ambiguous);fvm_context_destroy(c);fvm_runtime_destroy(rt);std::printf("PASS inferred matrix accepted=%u ambiguous=%u\n",accepted,ambiguous);return 0;
}
