/* Import OOM never publishes a partial graph; callback state survives errors and GC. */
#define main allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include "movieclip_sound.cpp"

int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK &&
           fvm_standard_install(context) == FVM_OK);
    Bytes swf = sound_movie();
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    bool failed = false, passed = false;
    FVM_Object *clip = 0;
    for (long boundary = 0; boundary < 4096 && !passed; ++boundary)
    {
        clip = (FVM_Object *)1;
        allowance = boundary;
        FVM_Result result = fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            assert(!clip);
            assert(fvm_gc_collect(context, 0) == FVM_OK);
        }
        else
        {
            assert(result == FVM_OK && clip);
            passed = true;
        }
    }
    assert(failed && passed);
    assert(fvm_gc_pin(context, clip) == FVM_OK);
    Delivery state;
    state.source = &swf;
    assert(fvm_swf_sound_mixer_create(44100, 1, 4, &state.mixer) == FVM_OK);
    allowance = 0;
    assert(fvm_movieclip_set_sound_callback(context, receive, &state) == FVM_OK);
    assert(fvm_movieclip_set_sound_callback(context, 0, 0) == FVM_OK);
    allowance = -1;
    assert(fvm_movieclip_set_sound_callback(context, receive, &state) == FVM_OK);
    assert(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals.size() == 2);
    fvm_gc_unpin(context, clip);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    fvm_swf_sound_mixer_close(state.mixer);
    return 0;
}
