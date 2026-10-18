/* The ordinal/source graph travels, but host callbacks and mixer pointers never do. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include "movieclip_sound.cpp"
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *source = 0, *target = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &source) == FVM_OK &&
           fvm_context_create(runtime, &target) == FVM_OK);
    assert(fvm_standard_install(source) == FVM_OK && fvm_standard_install(target) == FVM_OK);
    Bytes swf = sound_movie();
    assert(fvm_load_memory(source, swf.begin(), swf.size()) == FVM_OK);
    assert(fvm_load_memory(target, swf.begin(), swf.size()) == FVM_OK);
    FVM_Object *clip = 0;
    assert(fvm_movieclip_load_swf(source, swf.begin(), swf.size(), "root", &clip) == FVM_OK);
    Delivery state;
    state.source = &swf;
    assert(fvm_swf_sound_mixer_create(44100, 1, 4, &state.mixer) == FVM_OK);
    assert(fvm_movieclip_set_sound_callback(source, receive, &state) == FVM_OK);
    FVM_Value value, restored;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = clip;
    unsigned char *data = 0;
    size_t size = 0;
    assert(fvm_value_serialize(source, &value, &data, &size) == FVM_OK);
    assert(fvm_value_deserialize(target, data, size, &restored) == FVM_OK);
    fvm_serialized_free(data);
    fvm_context_destroy(source);
    clip = restored.object;
    assert(fvm_gc_pin(target, clip) == FVM_OK && fvm_gc_collect(target, 0) == FVM_OK);
    assert(fvm_movieclip_advance(target, clip, 1) == FVM_UNSUPPORTED && state.ordinals.empty());
    assert(fvm_movieclip_set_sound_callback(target, receive, &state) == FVM_OK);
    seek(target, clip, 1, false);
    assert(fvm_movieclip_advance(target, clip, 1) == FVM_OK && state.ordinals.size() == 2);
    fvm_gc_unpin(target, clip);
    fvm_context_destroy(target);
    fvm_runtime_destroy(runtime);
    fvm_swf_sound_mixer_close(state.mixer);
    return 0;
}
