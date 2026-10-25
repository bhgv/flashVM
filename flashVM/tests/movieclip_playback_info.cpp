/* Native playback observations survive replaced getters, stopped ticks and zero-allocation use. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "../src/standard/movieclip_state.hpp"
#include "../src/standard/movieclip_constants.h"
#include <cstring>


static void call(FVM_Context *c, FVM_Object *o, const char *name, unsigned target = 0)
{
    FVM_Value v, a;
    fvm_value_number(&a, target);
    if (!(fvm_object_call(c, o, name, target ? 1 : 0, target ? &a : 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *r = 0;
    FVM_Context *c = 0, *foreign = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(r, &c) == FVM_OK && fvm_context_create(r, &foreign) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *o = 0, *plain = 0;
    if (!(fvm_movieclip_create(c, "root", 4, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &plain) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_MovieClipPlaybackInfo info;
    std::memset(&info, 0x5a, sizeof(info));
    info.struct_size = sizeof(info);
    FVM_MovieClipPlaybackInfo before = info;
    if (!(fvm_movieclip_get_playback_info(0, o, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_playback_info(foreign, o, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_playback_info(c, plain, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_playback_info(c, o, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = 0;
    before = info;
    if (!(fvm_movieclip_get_playback_info(c, o, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    info.struct_size = sizeof(info);
    unsigned long token = 91;
    allowance = 0;
    if (!(fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK &&
          fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(info.frame == 1 && info.total_frames == 4 && info.playing && !info.entered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, o, 1) == FVM_OK && fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK &&
          info.frame == 1 && info.entered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, o, "stop");
    if (!(fvm_movieclip_advance(c, o, 2) == FVM_OK && fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK &&
          info.frame == 1 && info.entered && !info.playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, o, "gotoAndPlay", 3);
    if (!(fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK && info.frame == 3 && !info.entered && info.playing))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, o, 1) == FVM_OK && fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK &&
          info.frame == 3 && info.entered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, o, 2) == FVM_OK && fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK &&
          info.frame == 1 && info.entered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value fake;
    fvm_value_number(&fake, 999);
    o->properties["_currentframe"] = fake;
    allowance = 0;
    if (!(fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK && info.frame == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    FVM_Object *data = o->resource_result.object;
    FVM_Value old = data->properties["entered"];
    data->properties["entered"] = fake;
    before = info;
    if (!(fvm_movieclip_get_playback_info(c, o, &info) == FVM_INVALID_ARGUMENT &&
          !std::memcmp(&info, &before, sizeof(info))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    data->properties["entered"] = old;
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, o, "gotoAndPlay", 1);
    if (!(fvm_movieclip_advance(c, o, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK && info.frame == 1 && info.entered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 0;
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    call(c, o, "nextFrame");
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, o, "prevFrame");
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, o, "play");
    call(c, o, "stop");
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value label, result;
    if (!(fvm_value_string(&label, "missing-label") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, o, "gotoAndPlay", 1, &label, &result) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    token = 91;
    if (!(fvm_movieclip_get_navigation(foreign, o, &token) == FVM_INVALID_ARGUMENT && token == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_navigation(c, o, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&data->properties[FVM_MOVIECLIP_NAVIGATION_KEY], FVM_MOVIECLIP_NAVIGATION_MAX);
    call(c, o, "gotoAndPlay", 1);
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value saved = data->properties[FVM_MOVIECLIP_NAVIGATION_KEY];
    fvm_value_number(&data->properties[FVM_MOVIECLIP_NAVIGATION_KEY], -1);
    token = 91;
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_INVALID_ARGUMENT && token == 91))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    data->properties[FVM_MOVIECLIP_NAVIGATION_KEY] = saved;
    data->properties.erase(FVM_MOVIECLIP_NAVIGATION_KEY);
    allowance = 0;
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_note_navigation(data) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_get_navigation(c, o, &token) == FVM_OK && token == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    if (!(fvm_gc_step(c, 0) == FVM_OK && fvm_movieclip_get_playback_info(c, o, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(foreign);
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    return 0;
}
