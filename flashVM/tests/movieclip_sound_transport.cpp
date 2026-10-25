/* The ordinal/source graph travels, but host callbacks and mixer pointers never do. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_sound.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *source = 0, *target = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &source) == FVM_OK && fvm_context_create(runtime, &target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(source) == FVM_OK && fvm_standard_install(target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = sound_movie();
    if (!(fvm_load_memory(source, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(target, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf(source, swf.begin(), swf.size(), "root", &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Delivery state;
    state.source = &swf;
    if (!(fvm_swf_sound_mixer_create(44100, 1, 4, &state.mixer) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_set_sound_callback(source, receive, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value, restored;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = clip;
    unsigned char *data = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(source, &value, &data, &size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(target, data, size, &restored) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(data);
    fvm_context_destroy(source);
    clip = restored.object;
    if (!(fvm_gc_pin(target, clip) == FVM_OK && fvm_gc_collect(target, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(target, clip, 1) == FVM_UNSUPPORTED && state.ordinals.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_set_sound_callback(target, receive, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    seek(target, clip, 1, false);
    if (!(fvm_movieclip_advance(target, clip, 1) == FVM_OK && state.ordinals.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_gc_unpin(target, clip);
    fvm_context_destroy(target);
    fvm_runtime_destroy(runtime);
    fvm_swf_sound_mixer_close(state.mixer);
    return 0;
}
