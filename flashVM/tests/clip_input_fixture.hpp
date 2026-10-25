/* Reuse independently encoded SWF placement handlers to exercise whole-scene input delivery. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_actions_fixture.hpp"
#include "flashvm/input.h"
#include "flashvm/keyboard.h"


static FVM_Object *input_actor(FVM_Context *c, FVM_Object **out = 0)
{
    FVM_Object *root = clip_load(c, clip_movie(8, clip_event_table(8, 0xf8, action(42))));
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (out)
    {
        *out = root;
    }
    return get(c, root, "actor").object;
}


static FVM_Object *input_duplicate(FVM_Context *c, FVM_Object *clip, const char *name, unsigned depth)
{
    FVM_Value a[2], out;
    if (!(fvm_value_string(a, name) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(a + 1, depth);
    if (!(fvm_object_call(c, clip, "duplicateMovieClip", 2, a, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
}


static void input_clean(FVM_Context *c)
{
    clip_clean(c);
    if (!(!c->clip_input_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
