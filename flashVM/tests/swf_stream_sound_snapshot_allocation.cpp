/* Every snapshot allocation failure is recoverable; prepared reads survive VM teardown without allocation. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define main stream_reference_main
#include "swf_stream_sound.cpp"
#undef main
#include "flashvm/swf_stream_pcm.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 Bytes tags,pcm;pcm.push_back(255);tag(tags,45,header(12));tag(tags,19,pcm);load(c,movie(tags));
 FVM_SWFStreamSoundSnapshot *s=0;
 for(long boundary=0;boundary<3;++boundary){allowance=boundary;assert(fvm_swf_stream_sound_snapshot_open(c,0,&s)==FVM_OUT_OF_MEMORY && !s);allowance=-1;}
 struct IndexShape { FVM_SWFStreamSoundBlock block;size_t offset; };
 rt->config.memory_limit_bytes=(unsigned long)(sizeof(IndexShape));
 assert(fvm_swf_stream_sound_snapshot_open(c,0,&s)==FVM_OUT_OF_MEMORY && !s);
 rt->config.memory_limit_bytes=(unsigned long)(sizeof(IndexShape)+pcm.size());
 assert(fvm_swf_stream_sound_snapshot_open(c,0,&s)==FVM_OK);
 assert(fvm_gc_collect(c,0)==FVM_OK);
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
 allowance=0;FVM_SWFStreamSoundInfo info;info.struct_size=sizeof(info);FVM_SWFStreamSoundBlock block;block.struct_size=sizeof(block);size_t n=0;short out=0;
 assert(fvm_swf_stream_sound_snapshot_info(s,&info)==FVM_OK);
 assert(fvm_swf_stream_sound_snapshot_block_count(s,&n)==FVM_OK && n==1);
 assert(fvm_swf_stream_sound_snapshot_block_at(s,0,&block)==FVM_OK && block.data[0]==255);
 assert(fvm_swf_stream_sound_snapshot_read_pcm(s,0,0,1,&out,1,&n)==FVM_OK && n==1 && out==32512);
 FVM_SWFStreamPCMReader reader;
 assert(fvm_swf_stream_pcm_reader_init(&reader,s)==FVM_OK);
 FVM_SWFStreamPCMChunk chunk;chunk.struct_size=sizeof(chunk);
 assert(fvm_swf_stream_pcm_reader_next(&reader,1,&out,1,&chunk,&n)==FVM_OK && n==1 && out==32512);
 assert(fvm_swf_stream_pcm_reader_seek(&reader,0,0)==FVM_OK);
 assert(fvm_swf_stream_pcm_reader_next(&reader,1,&out,1,&chunk,&n)==FVM_OK && n==1);
 assert(fvm_swf_stream_pcm_reader_next(&reader,1,&out,1,&chunk,&n)==FVM_NOT_FOUND);
 fvm_swf_stream_sound_snapshot_close(s);allowance=-1;
 return 0;
}
