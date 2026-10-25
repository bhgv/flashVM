/* Deep prepared copies preserve styled progress and remain independent after source teardown. */
#define main styled_voice_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_voice.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes data;
    for (unsigned i = 0; i < 5; ++i)
    {
        word(data, (unsigned short)(1000 + i * 500));
    }
    Bytes swf = file(0x3e, 5, data, 8);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *v = 0, *copy = 0;
    if (!(fvm_swf_sound_voice_open(c, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    FVM_SWFSoundVoiceStorageInfo storage;
    storage.struct_size = sizeof(storage);
    if (!(fvm_swf_sound_voice_storage_info(v, &storage) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    short left[32], right[32];
    size_t a = 0, b = 0;
    for (unsigned step = 0; step < 17; ++step)
    {
        copy = (FVM_SWFSoundVoice *)1;
        if (!(fvm_swf_sound_voice_clone(v, storage.retained_bytes - 1, &copy) == FVM_OUT_OF_MEMORY && !copy))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_sound_voice_clone(v, storage.retained_bytes, &copy) == FVM_OK && copy))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFSoundVoiceInfo x, y;
        x.struct_size = sizeof(x);
        y.struct_size = sizeof(y);
        if (!(fvm_swf_sound_voice_info(v, &x) == FVM_OK && fvm_swf_sound_voice_info(copy, &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(x.play_index == y.play_index && x.frame_in_play == y.frame_in_play && x.finished == y.finished))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Result l = fvm_swf_sound_voice_read(v, 1, left, 32, &a);
        FVM_Result r = fvm_swf_sound_voice_read(copy, 1, right, 32, &b);
        if (!(l == r && a == b && !std::memcmp(left, right, a * sizeof(short))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (step != 16)
        {
            fvm_swf_sound_voice_close(copy);
        }
    }
    fvm_swf_sound_voice_close(v);
    if (!(fvm_swf_sound_voice_read(copy, 1, right, 32, &b) == FVM_NOT_FOUND && !b))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_voice_close(copy);
    copy = (FVM_SWFSoundVoice *)1;
    if (!(fvm_swf_sound_voice_clone(0, (size_t)-1, &copy) == FVM_INVALID_ARGUMENT && !copy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_clone(0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
