/* Every failed parser/build and dispatch allocation preserves cleanup and leaves future calls usable. */
#define main clip_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_allocation.cpp"

#undef main
#include "clip_actions_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = clip_movie(8, clip_event_table(8, 3, action(42)));
    bool success = false;
    unsigned failures = 0;
    for (long n = 0; n < 4096 && !success; ++n)
    {
        FVM_Context *c = clip_context(rt);
        FVM_Object *root = (FVM_Object *)1;
        allowance = n;
        FVM_Result r = fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root);
        allowance = -1;
        if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip_clean(c);
        if (r == FVM_OK)
        {
            success = true;
            if (!(root))
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
            ++failures;
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    success = false;
    failures = 0;
    for (long n = 0; n < 1024 && !success; ++n)
    {
        FVM_Context *c = clip_context(rt);
        FVM_Object *root = clip_load(c, swf);
        allowance = n;
        FVM_Result r = fvm_movieclip_advance(c, root, 1);
        allowance = -1;
        if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip_clean(c);
        if (r == FVM_OK)
        {
            success = true;
        }
        else
        {
            ++failures;
            if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *actor = get(c, root, "actor").object;
        number(c, actor, "seen", 42);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    success = false;
    failures = 0;
    for (long n = 0; n < 512 && !success; ++n)
    {
        FVM_Context *c = clip_context(rt);
        FVM_Object *root = clip_load(c, swf);
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(c, root, "actor").object;
        allowance = n;
        FVM_Result r = fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0);
        allowance = -1;
        if (!(r == FVM_OK || r == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip_clean(c);
        if (r == FVM_OK)
        {
            success = true;
        }
        else
        {
            ++failures;
            if (!(fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        number(c, actor, "seen", 42);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
}
