/* Long record geometry and every truncated canonical span fail before publishing PCM or counts. */
#include "../src/core/runtime.hpp"
#define main stream_bounds_reference_main
#include "swf_stream_sound.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 Bytes tags,pcm;for(unsigned i=0;i<200;++i)pcm.push_back((unsigned char)i);
 tag(tags,45,header(12));tag(tags,19,pcm);Bytes full=movie(tags);load(c,full);
 FVM_SWFStreamSoundBlock block;block.struct_size=sizeof(block);short out[3];size_t n=0,count=0;
 assert(fvm_swf_stream_sound_block_at(c,0,0,&block)==FVM_OK && block.sample_count==200 && block.data_size==200);
 assert(fvm_swf_stream_sound_read_pcm(c,0,0,197,3,out,3,&n)==FVM_OK && n==3);
 for(unsigned i=0;i<3;++i)assert(out[i]==(short)(((long)(197+i)-128)*256));
 Bytes canonical=c->swf_bytes;
 /* Internal fault fixture covers parser bounds that the outer load normally rejects first. */
 for(size_t cut=0;cut<canonical.size();++cut)
 {
  c->swf_bytes=canonical;c->swf_bytes.resize(cut);count=99;n=99;out[0]=42;
  assert(fvm_swf_stream_sound_count(c,&count)==FVM_INVALID_ARGUMENT && count==99);
  assert(fvm_swf_stream_sound_read_pcm(c,0,0,0,1,out,3,&n)==FVM_INVALID_ARGUMENT && !n && out[0]==42);
 }
 c->swf_bytes=canonical;
 for(unsigned i=22;i<26;++i)c->swf_bytes[i]=255;
 count=99;assert(fvm_swf_stream_sound_count(c,&count)==FVM_INVALID_ARGUMENT && count==99);
 c->swf_bytes=canonical;
 out[0]=42;n=99;assert(fvm_swf_stream_sound_read_pcm(c,0,0,0,(size_t)-1,out,(size_t)-1,&n)==FVM_INVALID_ARGUMENT && !n && out[0]==42);
 /* Failed outer reload preserves previous canonical data and ordinals. */
 Bytes truncated=full;truncated.resize(full.size()-1);
 assert(fvm_load_memory(c,truncated.begin(),truncated.size())!=FVM_OK);
 assert(fvm_swf_stream_sound_block_count(c,0,&count)==FVM_OK && count==1);
 assert(fvm_swf_stream_sound_read_pcm(c,0,0,0,1,out,3,&n)==FVM_OK && n==1 && out[0]==-32768);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
