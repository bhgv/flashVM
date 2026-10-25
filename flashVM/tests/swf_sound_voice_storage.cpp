/* Exact requested storage ceilings for native PCM and all analytical MPEG geometries. */
#define main mp3_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sound_mp3.cpp"

#undef main
#include "flashvm/gc.h"


static void check(FVM_Context *c, size_t pcm_bytes)
{
    FVM_SWFSoundVoice *voice = 0;
    if (!(fvm_swf_sound_voice_open(c, 0, &voice) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoiceStorageInfo storage = {sizeof(storage), 0, 0, 0};
    if (!(fvm_swf_sound_voice_storage_info(voice, &storage) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(storage.pcm_bytes == pcm_bytes && !storage.envelope_bytes && storage.retained_bytes > pcm_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoiceStorageInfo saved = storage;
    storage.struct_size = 0;
    if (!(fvm_swf_sound_voice_storage_info(voice, &storage) == FVM_INVALID_ARGUMENT && !storage.struct_size &&
          storage.pcm_bytes == saved.pcm_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_storage_info(0, &storage) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundVoice *other = (FVM_SWFSoundVoice *)1;
    if (!(fvm_swf_sound_voice_open_bounded(c, 0, 0, &other) == FVM_OUT_OF_MEMORY && !other))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_open_bounded(c, 0, saved.retained_bytes - 1, &other) == FVM_OUT_OF_MEMORY && !other))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_open_bounded(c, 0, saved.retained_bytes, &other) == FVM_OK && other))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    storage.struct_size = sizeof(storage);
    if (!(fvm_swf_sound_voice_storage_info(other, &storage) == FVM_OK &&
          storage.retained_bytes == saved.retained_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_voice_open_bounded(c, 0, saved.retained_bytes, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_voice_close(other);
    fvm_swf_sound_voice_close(voice);
}


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
    for (size_t k = 0; k < sizeof(mp3_fixture::fixtures) / sizeof(mp3_fixture::fixtures[0]); ++k)
    {
        const mp3_fixture::Fixture &f = mp3_fixture::fixtures[k];
        Bytes payload, tags, e;
        word(payload, 0);
        for (size_t i = 0; i < f.size; ++i)
        {
            payload.push_back(f.data[i]);
        }
        tag(tags, 14, sound(7, 0x22, (unsigned long)f.frames, payload.begin(), payload.size()));
        word(e, 7);
        e.push_back(0);
        tag(tags, 15, e);
        Bytes swf = movie(tags);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        check(c, f.frames * f.channels * sizeof(short));
    }
    for (unsigned ch = 1; ch <= 2; ++ch)
    {
        Bytes data, tags, e;
        for (unsigned i = 0; i < 12 * ch; ++i)
        {
            word(data, (unsigned short)i);
        }
        tag(tags, 14, sound(7, 0x3e | (ch - 1), 12, data.begin(), data.size()));
        word(e, 7);
        e.push_back(0);
        tag(tags, 15, e);
        Bytes swf = movie(tags);
        if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        check(c, 12 * ch * sizeof(short));
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
