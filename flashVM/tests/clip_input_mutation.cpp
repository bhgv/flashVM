/* New children wait, removed siblings skip, and captured ancestry prevents moved-subtree delivery. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_input_fixture.hpp"

static FVM_Object *root, *first, *victim, *newborn;
static unsigned calls;


static FVM_Result mutate(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    if (!(c->clip_input_dispatching && fvm_movieclip_dispatch_clip_input(c, root, 8) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (calls == 1)
    {
        if (!(fvm_object_call(c, victim, "removeMovieClip", 0, 0, out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        newborn = input_duplicate(c, first, "newborn", 5);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = clip_context(rt);
    Bytes code;
    integer(code, 0);
    text(code, "mutate");
    code.push_back(0x3d);
    code.push_back(0x17);
    code.push_back(0);
    root = clip_load(c, clip_movie(8, clip_event_table(8, 8, code)));
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    first = get(c, root, "actor").object;
    victim = input_duplicate(c, first, "victim", 3);
    if (!(fvm_register_function(c, "mutate", mutate, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    calls = 0;
    if (!(fvm_movieclip_dispatch_clip_input(c, root, 8) == FVM_OK && calls == 1 && newborn))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_clip_input(c, root, 8) == FVM_OK && calls == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input_clean(c);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
