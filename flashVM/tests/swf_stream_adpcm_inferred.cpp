/* Unique stereo five-bit partial ADPCM works without counts; legacy APIs retain their policy. */
#include "swf_stream_adpcm_inferred_fixture.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);
 Bits bits=partial_bits(5,2,17);Bytes swf=swf_block(bits.bytes,2);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
 FVM_SWFStreamSoundSnapshot *s=0;assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c,0,(size_t)-1,&s)==FVM_UNSUPPORTED&&!s);
 assert(fvm_swf_stream_sound_snapshot_open_pcm_inferred(c,0,(size_t)-1,&s)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);verify_partial(s,17,2,5);fvm_swf_stream_sound_snapshot_close(s);return 0;
}
