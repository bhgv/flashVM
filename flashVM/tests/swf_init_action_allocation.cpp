/* Initialization progress and reentry guards survive every allocation failure. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_allocation.cpp"

#undef main
#define main sprite_fixture_main
#include "swf_sprite_resources.cpp"

#undef main
#include "../src/core/runtime.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes empty, controls, tags;
    tag(controls, 0, empty);
    tag(tags, 39, sprite(7, 0, controls));
    Bytes init;
    u16(init, 7);
    Bytes code = action(18);
    for (size_t i = 0; i < code.size(); ++i)
    {
        init.push_back(code[i]);
    }
    tag(tags, 59, init);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 1);
    bool completed = false;
    for (long fail = 0; fail < 4096 && !completed; ++fail)
    {
        FVM_Object *root = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *library = root->resource_result.object->properties["library"].object;
        allowance = fail;
        FVM_Result result = fvm_movieclip_advance(c, root, 1);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!library->swf_initializer_running && library->swf_initializer_next <= 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_native_depth && !c->script_depth && !c->script_callee && !c->script_this && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && library->swf_initializer_next == 1 &&
              !library->swf_initializer_running))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (result == FVM_OK)
        {
            number(c, root, "seen", 18);
            completed = true;
        }
        if (!(fvm_gc_unpin(c, root) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(completed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
