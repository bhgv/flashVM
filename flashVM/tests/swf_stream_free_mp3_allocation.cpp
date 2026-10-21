/* Fail all native MP3 preparation acquisitions; normalized reads remain allocator-free after VM teardown. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define main stream_mp3_reference_main
#include "swf_stream_free_mp3.cpp"
#undef main
int main(int argc,char **argv)
{
    long first=0,last=5;
    if(argc==2){char *end=0;first=std::strtol(argv[1],&end,10);assert(end && !*end && first>=0 && first<=4);last=first+1;}else assert(argc==1);
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes swf=stream_movie(free_mp3_fixture::fixtures[0],1105);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFStreamSoundSnapshot *source=0;
    allowance=1;
    assert(fvm_swf_stream_sound_snapshot_open_pcm_bounded(c,0,0,&source)==FVM_OUT_OF_MEMORY && !source && allowance==1);
    allowance=-1;
    for(long budget=first;budget<last;++budget){allowance=budget;FVM_Result r=fvm_swf_stream_sound_snapshot_open_pcm(c,0,&source);allowance=-1;assert(budget<4?r==FVM_OUT_OF_MEMORY && !source:r==FVM_OK && source);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    if(source){allowance=0;FVM_SWFStreamPCMReader reader;assert(fvm_swf_stream_pcm_reader_init(&reader,source)==FVM_OK);short pcm[62];size_t total=0,written;for(;;){FVM_SWFStreamPCMChunk chunk;chunk.struct_size=sizeof(chunk);FVM_Result r=fvm_swf_stream_pcm_reader_next(&reader,31,pcm,62,&chunk,&written);if(r==FVM_NOT_FOUND)break;assert(r==FVM_OK);total+=chunk.frames;}assert(total==5760-1105);fvm_swf_stream_sound_snapshot_close(source);assert(allowance==0);allowance=-1;}
    return 0;
}
