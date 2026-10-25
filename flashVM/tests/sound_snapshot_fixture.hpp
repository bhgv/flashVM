/* Root sound-list fixtures; native notifications are observed without opening an audio device. */
#define FVM_MOVIECLIP_SOUND_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_sound.cpp"
#include "movieclip_tick_fixture.hpp"
#include "../src/standard/movieclip_sound.hpp"
#include "../src/standard/movieclip_sound_constants.h"
#include "../src/core/native_call_guard.hpp"


struct SoundSnapshot
{
    FVM_Object *list;
    unsigned count;
    size_t delivered[8];
    unsigned mutation;
    FVM_Result status;


    SoundSnapshot() : list(0), count(0), mutation(0), status(FVM_OK)
    {
    }
};


static FVM_Result sound_snapshot_receive(FVM_Context *c, FVM_Object *clip, size_t ordinal, void *raw)
{
    SoundSnapshot *p = static_cast<SoundSnapshot *>(raw);
    if (!(p->count < 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    p->delivered[p->count++] = ordinal;
    if (p->count == 1)
    {
        if (p->mutation == 1)
        {
            p->list->indexed[1] = tick_object(clip);
        }
        if (p->mutation == 2)
        {
            p->list->indexed.clear();
        }
        if (p->mutation == 3)
        {
            FVM_Value value;
            fvm_value_number(&value, 4);
            p->list->indexed[2] = value;
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_set_sound_callback(c, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(c->movieclip_sound_busy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return p->status;
}


static FVM_Object *sound_snapshot_root(FVM_Context *c, SoundSnapshot &p, unsigned version = 8)
{
    Bytes swf = sound_movie();
    swf[3] = (unsigned char)version;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *frames = clip->resource_result.object->properties["frames"].object;
    p.list = frames->indexed[1].object->properties["sounds"].object;
    if (!(fvm_movieclip_set_sound_callback(c, sound_snapshot_receive, &p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return clip;
}


static FVM_Result sound_snapshot_run(FVM_Context *c, FVM_Object *clip)
{


    FVM_NativeCallGuard guard(c);
    return fvm_movieclip_sound_run(c, clip, clip->resource_result.object->properties["frames"].object, 1);
}
