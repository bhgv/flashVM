/* Independent SWF wire fixtures: placement event flags and action ranges are built without parser constants. */
#define main clip_sprite_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_resources.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/clip_actions.h"
#include "flashvm/stage.h"


static void clip_flags(Bytes &b, unsigned version, unsigned long flags)
{
    if (version < 6)
    {
        u16(b, (unsigned)flags);
    }
    else
    {
        u32(b, flags);
    }
}


static void clip_record(Bytes &b, unsigned version, unsigned long flags, const Bytes &code, unsigned key = 0)
{
    clip_flags(b, version, flags);
    u32(b, (unsigned long)code.size() + ((flags & 0x20000) ? 1 : 0));
    if (flags & 0x20000)
    {
        b.push_back((unsigned char)key);
    }
    for (size_t i = 0; i < code.size(); ++i)
    {
        b.push_back(code[i]);
    }
}


static Bytes clip_event_table(unsigned version, unsigned long flags, const Bytes &code, unsigned key = 0)
{
    Bytes b;
    u16(b, 0);
    clip_flags(b, version, flags);
    clip_record(b, version, flags, code, key);
    clip_flags(b, version, 0);
    return b;
}


static Bytes clip_place(const Bytes &events, bool modify = false)
{
    Bytes b;
    b.push_back(modify ? 0x81 : 0xa6);
    u16(b, 1);
    if (!modify)
    {
        u16(b, 7);
        b.push_back(0);
        name(b, "actor");
    }
    for (size_t i = 0; i < events.size(); ++i)
    {
        b.push_back(events[i]);
    }
    return b;
}


static Bytes clip_movie(unsigned version, const Bytes &events, const Bytes *update = 0)
{
    Bytes tags, controls, empty;
    tag(controls, 1, empty);
    tag(controls, 0, empty);
    tag(tags, 39, sprite(7, 1, controls));
    tag(tags, 26, clip_place(events));
    tag(tags, 1, empty);
    if (update)
    {
        tag(tags, 26, clip_place(*update, true));
        tag(tags, 1, empty);
    }
    tag(tags, 0, empty);
    Bytes swf = movie(tags, update ? 2 : 1);
    swf[3] = (unsigned char)version;
    return swf;
}


static FVM_Context *clip_context(FVM_Runtime *rt)
{
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return c;
}


static FVM_Object *clip_load(FVM_Context *c, const Bytes &swf, FVM_Result expected = FVM_OK)
{
    FVM_Object *root = (FVM_Object *)1;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected == FVM_OK)
    {
        if (!(root && fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(!root))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return root;
}


static void clip_clean(FVM_Context *c)
{
    if (!(!c->gc_native_depth && !c->gc_frames && !c->avm1_execution_depth && !c->script_depth &&
          !c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
