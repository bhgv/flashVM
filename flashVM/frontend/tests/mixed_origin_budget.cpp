/* A deliberately undersized adapter ceiling rejects mixed preflight before any SDL endpoint. */
#define FVM_DESKTOP_MIXED_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "mixed_open.cpp"


int main()
{
    size_t old_bytes = 0, new_bytes = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required_for_tracking(2, FVM_DESKTOP_AUDIO_BLOCK, 1, &old_bytes) == FVM_OK &&
          fvm_swf_sound_mix_queue_storage_required_for_modes(2, FVM_DESKTOP_AUDIO_BLOCK, 1, 1, &new_bytes) == FVM_OK &&
          old_bytes <= FVM_DESKTOP_AUDIO_MIX_BYTES && new_bytes > FVM_DESKTOP_AUDIO_MIX_BYTES))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = mixed_movie();
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK && fvm_desktop_audio_validate(c) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!SDL_WasInit(SDL_INIT_AUDIO)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf_ex(c, swf.begin(), swf.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) ==
              FVM_OK &&
          fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_DesktopAudio a;
    if (!(fvm_desktop_audio_open_paused(c, clip, &a, 1) == FVM_OUT_OF_MEMORY && !a.device && !a.thread &&
          !a.composition && !a.stop_binding))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_audio_close(&a);
    if (!(!SDL_WasInit(SDL_INIT_AUDIO)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_gc_unpin(c, clip);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
