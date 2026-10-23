/* Invalid bit endings and ambiguous zero codes never silently become a guessed short packet. */
#include "swf_stream_adpcm_inferred_fixture.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);
 FVM_SWFStreamSoundSnapshot *source=reinterpret_cast<FVM_SWFStreamSoundSnapshot*>(1);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(0,0,999,&source)==FVM_INVALID_ARGUMENT&&!source);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,999,0)==FVM_INVALID_ARGUMENT);
 Bits bits=partial_bits(5,2,17);bits.bytes[bits.bytes.size()-1]|=1;Bytes swf=swf_block(bits.bytes,2);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source)==FVM_INVALID_ARGUMENT&&!source);
 Bytes short_header;short_header.push_back(0xc0);short_header.push_back(0);swf=swf_block(short_header,2);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source)==FVM_INVALID_ARGUMENT&&!source);
 bits=partial_bits(2,1,2);swf=swf_block(bits.bytes,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source)==FVM_UNSUPPORTED&&!source);
 /* A genuine final nonzero code removes shorter endpoint candidates; retain preceding zero codes. */
 bits.bytes[bits.bytes.size()-1]|=1;swf=swf_block(bits.bytes,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&source)==FVM_OK);
 short pcm[5];size_t n=0;assert(fvm_swf_stream_sound_snapshot_read_pcm(source,0,0,5,pcm,5,&n)==FVM_OK&&n==5);
 const short expected[]={100,103,106,109,119};assert(!memcmp(pcm,expected,sizeof(pcm)));fvm_swf_stream_sound_snapshot_close(source);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,1,(size_t)-1,&source)==FVM_NOT_FOUND&&!source);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
