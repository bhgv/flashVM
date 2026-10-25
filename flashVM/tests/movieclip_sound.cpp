/* Root sound entry, pause/seek/loop routing and interpreter/JIT share one actual frame path. */
#define main timeline_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_movieclip_timeline.cpp"

#undef main
#include "flashvm/swf_sound_mixer.h"
#include "flashvm/execution_mode.h"
#include <new>


static void sound_event(Bytes &tags, unsigned flags)
{
    Bytes data;
    u16(data, 7);
    data.push_back((unsigned char)flags);
    tag(tags, 15, data);
}


static Bytes sound_movie()
{
    Bytes tags, pcm, empty;
    u16(pcm, 7);
    pcm.push_back(12);
    u32(pcm, 2);
    pcm.push_back(0);
    pcm.push_back(255);
    tag(tags, 14, pcm);
    sound_event(tags, 0);
    sound_event(tags, 16);
    Bytes first = action(1);
    first[first.size() - 1] = 7;
    first.push_back(0); /* Stop after callback and seen=1. */
    tag(tags, 12, first);
    tag(tags, 1, empty);
    sound_event(tags, 0);
    tag(tags, 12, action(2));
    tag(tags, 1, empty);
    sound_event(tags, 32);
    tag(tags, 12, action(3));
    tag(tags, 1, empty);
    sound_event(tags, 0);
    tag(tags, 12, action(4));
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    return movie(tags, 4);
}


struct Delivery
{
    FVM_SWFSoundMixer *mixer;
    fvm_detail::vector<size_t> ordinals;
    FVM_Result failure;
    int throws;
    const Bytes *source;


    Delivery() : mixer(0), failure(FVM_OK), throws(0), source(0)
    {
    }
};


static FVM_Result receive(FVM_Context *c, FVM_Object *clip, size_t ordinal, void *raw)
{
    Delivery *state = (Delivery *)raw;
    state->ordinals.push_back(ordinal);
    if (!(fvm_movieclip_advance(c, clip, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_set_sound_callback(c, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(c, state->source->begin(), state->source->size()) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Context destruction is already blocked by the native-call barrier. */
    fvm_context_destroy(c);
    if (state->throws == 1)
    {
        throw std::bad_alloc();
    }
    if (state->throws == 2)
    {
        throw 7;
    }
    if (state->failure != FVM_OK)
    {
        return state->failure;
    }
    return fvm_swf_sound_mixer_submit(state->mixer, c, ordinal);
}


static void seek(FVM_Context *c, FVM_Object *clip, unsigned frame, bool play)
{
    FVM_Value arg, result;
    fvm_value_number(&arg, frame);
    if (!(fvm_object_call(c, clip, play ? "gotoAndPlay" : "gotoAndStop", 1, &arg, &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static unsigned voices(Delivery &state)
{
    FVM_SWFSoundMixerInfo info;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_sound_mixer_info(state.mixer, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return info.active_voices;
}


static void run(FVM_ExecutionMode mode)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = sound_movie();
    FVM_Object *clip = (FVM_Object *)1;
    if (!(fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip) == FVM_INVALID_ARGUMENT && !clip))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result imported = fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip);
    if (imported != FVM_OK)
    {
        std::fprintf(stderr, "import status %d\n", (int)imported);
    }
    if (!(imported == FVM_OK && clip))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(context, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_set_mode(context, mode, 0) == FVM_OK))
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
    if (!(fvm_movieclip_set_sound_callback(context, receive, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state.ordinals.size() == 2 && state.ordinals[0] == 0 && state.ordinals[1] == 1 && voices(state) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(context, clip, "seen", 1);
    if (!(fvm_movieclip_advance(context, clip, 3) == FVM_OK && state.ordinals.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Seek reconstruction walks frames 1..3 visually, but only frame 3's stop is delivered. */
    seek(context, clip, 3, true);
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state.ordinals.size() == 3 && state.ordinals[2] == 3 && !voices(state)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(context, clip, "seen", 3);
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals[3] == 4 && voices(state) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals.size() == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(context, clip, "seen", 1);
    seek(context, clip, 1, false);
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals.size() == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value unused;
    if (!(fvm_object_call(context, clip, "play", 0, 0, &unused) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals[8] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(context, clip, "seen", 2);
    FVM_ExecutionStats stats;
    stats.struct_size = sizeof(stats);
    if (!(fvm_execution_get_stats(context, &stats) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (mode == FVM_EXECUTION_JIT)
    {
        if (!(stats.native_entries > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(stats.interpreter_entries > 0 && !stats.native_entries))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Missing callback and callback errors consume the entry, never silently suppress audio. */
    if (!(fvm_movieclip_set_sound_callback(context, 0, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    seek(context, clip, 3, false);
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_set_sound_callback(context, receive, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (int failure = 0; failure < 3; ++failure)
    {
        seek(context, clip, 3, false);
        state.failure = failure == 0 ? FVM_EXCEPTION : FVM_OK;
        state.throws = failure;
        FVM_Result expected = failure == 0 ? FVM_EXCEPTION : failure == 1 ? FVM_OUT_OF_MEMORY : FVM_ERROR;
        if (!(fvm_movieclip_advance(context, clip, 1) == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t count = state.ordinals.size();
        if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals.size() == count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_set_sound_callback(context, receive, &state) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    state.failure = FVM_OK;
    state.throws = 0;
    /* Wrong SWF with the same dictionary IDs must not reinterpret the stored ordinal. */
    Bytes changed = swf;
    changed[24] = 128;
    if (!(fvm_load_memory(context, changed.begin(), changed.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    seek(context, clip, 3, false);
    size_t count = state.ordinals.size();
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_INVALID_ARGUMENT && state.ordinals.size() == count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    seek(context, clip, 3, false);
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && !voices(state)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_gc_unpin(context, clip);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    fvm_swf_sound_mixer_close(state.mixer);
}

#ifndef FVM_MOVIECLIP_SOUND_FIXTURE_ONLY


int main()
{
    if (!(fvm_movieclip_set_sound_callback(0, receive, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    run(FVM_EXECUTION_INTERPRETER);
    if (fvm_jit_available())
    {
        run(FVM_EXECUTION_JIT);
    }
    return 0;
}

#endif
