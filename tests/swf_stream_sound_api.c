/* Strict C89 streaming sound ABI and compressed canonical-file equivalence. */
#include "flashvm/swf_stream_sound.h"
#include <assert.h>
static const unsigned char raw[]={70,87,83,6,42,0,0,0,8,0,0,12,4,0,68,11,6,61,1,0,196,4,0,255,128,128,64,0,64,0,194,4,255,0,64,0,192,4,64,0,0,0};
static const unsigned char compressed[]={67,87,83,6,42,0,0,0,120,156,227,96,96,224,97,97,112,225,102,179,101,100,56,194,194,240,191,161,193,129,193,129,225,16,203,127,32,121,128,197,129,129,129,1,0,106,71,6,252};
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;unsigned i;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 for(i=0;i<2;++i){FVM_SWFStreamSoundInfo info;FVM_SWFStreamSoundBlock block;size_t n=0,count=0;short out[4];
 assert(fvm_load_memory(c,i?compressed:raw,i?sizeof(compressed):sizeof(raw))==FVM_OK);
 info.struct_size=sizeof(info);block.struct_size=sizeof(block);
 assert(fvm_swf_stream_sound_count(c,&count)==FVM_OK && count==1);
 assert(fvm_swf_stream_sound_at(c,0,&info)==FVM_OK && info.format==3 && info.channels==2 && info.sample_rate==44100 && info.average_samples==1 && info.playback_sample_rate==11025 && info.playback_channels==1);
 assert(fvm_swf_stream_sound_block_count(c,0,&count)==FVM_OK && count==3);
 assert(fvm_swf_stream_sound_block_at(c,0,0,&block)==FVM_OK && block.frame==1 && block.sample_count==2 && block.data_size==4);
 assert(fvm_swf_stream_sound_read_pcm(c,0,0,0,2,out,4,&n)==FVM_OK && n==4 && out[0]==-32768 && out[1]==32512 && !out[2] && !out[3]);
 assert(fvm_swf_stream_sound_block_at(c,0,1,&block)==FVM_OK && block.frame==3 && block.sample_count==1);
 assert(fvm_swf_stream_sound_read_pcm(c,0,1,0,1,out,4,&n)==FVM_OK && n==2 && out[0]==32512 && out[1]==-32768);
 assert(fvm_swf_stream_sound_block_at(c,0,2,&block)==FVM_OK && block.frame==4 && !block.sample_count);
 assert(fvm_swf_stream_sound_read_pcm(c,0,2,0,0,0,0,&n)==FVM_OK && !n);
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
