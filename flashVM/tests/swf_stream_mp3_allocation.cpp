/* Compressed capture OOM/retry, read-only metadata under denied allocation and after VM teardown. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_MP3_FIXTURE_ONLY
#include "swf_stream_mp3.cpp"
#undef FVM_MP3_FIXTURE_ONLY
int main()
{
    FVM_Config config; FVM_Runtime *runtime=0; FVM_Context *context=0;
    assert(fvm_config_init(&config)==FVM_OK);
    assert(fvm_runtime_create(&config,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK);
    load(context,mp3_movie(8,45,1,576,65535));
    FVM_SWFStreamSoundSnapshot *snapshot=0;
    for(long boundary=0;boundary<3;++boundary)
    {
        allowance=boundary;
        assert(fvm_swf_stream_sound_snapshot_open(context,0,&snapshot)==FVM_OUT_OF_MEMORY && !snapshot);
        allowance=-1;
    }
    assert(fvm_swf_stream_sound_snapshot_open(context,0,&snapshot)==FVM_OK);
    FVM_SWFStreamSoundMP3Block view;view.struct_size=sizeof(view);
    allowance=0;
    assert(fvm_swf_stream_sound_mp3_block_at(context,0,0,&view)==FVM_OK && view.seek_samples==-1);
    context->gc_collecting=true;
    assert(fvm_swf_stream_sound_mp3_block_at(context,0,0,&view)==FVM_INVALID_ARGUMENT);
    context->gc_collecting=false;
    allowance=-1;
    assert(fvm_gc_collect(context,0)==FVM_OK);
    fvm_context_destroy(context);fvm_runtime_destroy(runtime);
    allowance=0;
    assert(fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot,0,&view)==FVM_OK);
    assert(view.declared_samples==576 && view.seek_samples==-1 && view.data_size==3);
    size_t n=99;short out=42;
    assert(fvm_swf_stream_sound_snapshot_read_pcm(snapshot,0,0,0,&out,1,&n)==FVM_UNSUPPORTED && !n && out==42);
    fvm_swf_stream_sound_snapshot_close(snapshot);allowance=-1;
    return 0;
}
