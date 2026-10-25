/* Reject malformed/ambiguous/sprite sound controls; FWS/CWS use the same canonical source. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_sound.cpp"


static Bytes definition()
{
    Bytes data;
    u16(data, 7);
    data.push_back(12);
    u32(data, 2);
    data.push_back(0);
    data.push_back(255);
    return data;
}


int main()
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
    for (unsigned scenario = 0; scenario < 5; ++scenario)
    {
        Bytes tags, empty;
        tag(tags, 14, definition());
        if (scenario == 0)
        {
            sound_event(tags, 192);
        }
        if (scenario == 1)
        {
            tag(tags, 14, definition());
        }
        if (scenario == 2)
        {
            Bytes placement;
            placement.push_back(2);
            u16(placement, 1);
            u16(placement, 7);
            tag(tags, 26, placement);
        }
        if (scenario == 3)
        {
            Bytes sprite;
            u16(sprite, 9);
            u16(sprite, 1);
            sound_event(sprite, 0);
            tag(sprite, 1, empty);
            tag(sprite, 0, empty);
            tag(tags, 39, sprite);
        }
        if (scenario == 4)
        {
            Bytes exports;
            u16(exports, 1);
            u16(exports, 7);
            exports.push_back('a');
            exports.push_back(0);
            tag(tags, 56, exports);
        }
        tag(tags, 1, empty);
        tag(tags, 0, empty);
        Bytes swf = movie(tags, 1);
        if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = (FVM_Object *)1;
        FVM_Result result = fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip);
        if (!(result == (scenario >= 3 ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT) && !clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes swf = sound_movie(), cws;
    for (size_t i = 0; i < 8; ++i)
    {
        cws.push_back(swf[i]);
    }
    cws[0] = 'C';
    uLongf bound = compressBound((uLong)swf.size() - 8);
    cws.resize(8 + bound);
    if (!(compress2(cws.begin() + 8, &bound, swf.begin() + 8, (uLong)swf.size() - 8, 9) == Z_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws.resize(8 + bound);
    if (!(fvm_load_memory(context, cws.begin(), cws.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf(context, swf.begin(), swf.size(), "root", &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(context, clip) == FVM_OK))
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
    if (!(fvm_movieclip_advance(context, clip, 1) == FVM_OK && state.ordinals.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_gc_unpin(context, clip);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    fvm_swf_sound_mixer_close(state.mixer);
    return 0;
}
