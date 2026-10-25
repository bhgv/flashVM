/* Raw StartDrag/EndDrag preserve operand order, context state and deferred positioning. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "flashvm/input.h"
#include "../src/core/runtime.hpp"


static void run(FVM_Context *c, FVM_Object *root, Bytes &b)
{
    b.push_back(0);
    FVM_GCRootFrame *roots = c->gc_frames;
    if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), root) == FVM_OK && c->gc_frames == roots &&
          !c->avm1_target_active && !c->avm1_execution_depth && !c->drag_updating && !c->gc_native_depth &&
          !c->script_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void position(FVM_Context *c, FVM_Object *clip, double x, double y)
{
    FVM_Affine m;
    if (!(fvm_movieclip_get_matrix(c, clip, &m) == FVM_OK && m.x0 == x && m.y0 == y))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    for (unsigned profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c = 0;
        FVM_Object *root = 0;
        FVM_Value a[2], out;
        FVM_Affine original = {1, 0, 0, 1, 5, 7};
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            swf[3] = (unsigned char)(profile + 3);
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(a, "clip");
        fvm_value_number(a + 1, 1);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = out.object;
        if (!(fvm_movieclip_set_matrix(c, clip, &original) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes b;
        text(b, "sentinel");
        integer(b, 77);
        integer(b, 0);
        integer(b, 0);
        integer(b, 10);
        integer(b, 10);
        integer(b, 1);
        integer(b, 1);
        text(b, "clip");
        b.push_back(0x27);
        b.push_back(0x1d);
        run(c, root, b);
        if (!(fvm_object_get(c, root, "sentinel", &out) == FVM_OK && out.number == 77 &&
              fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip && c->drag_lock_center &&
              c->drag_constrained))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 5, 7);
        if (!(fvm_input_set_pointer(c, 30, 40) == FVM_OK && fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 10, 10);
        if (!(fvm_input_set_pointer(c, 8, 9) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        b.push_back(0x28);
        run(c, root, b);
        position(c, clip, 8, 9);
        if (!(fvm_exec_drag_target(c, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_set_matrix(c, clip, &original) == FVM_OK && fvm_input_set_pointer(c, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        integer(b, 2);
        integer(b, 2);
        text(b, "clip");
        b.push_back(0x27);
        run(c, root, b);
        if (!(!c->drag_lock_center && !c->drag_constrained && fvm_input_set_pointer(c, 4, 6) == FVM_OK &&
              fvm_exec_drag_update(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        position(c, clip, 9, 13);
        b.clear();
        integer(b, 0);
        integer(b, 1);
        text(b, "missing");
        b.push_back(0x27);
        run(c, root, b);
        if (!(fvm_exec_drag_target(c, &out) == FVM_OK && out.object == clip && fvm_exec_drag_end(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0x27, 0};
        if (!(fvm_avm1_execute_receiver(c, empty, sizeof(empty), root) == (profile ? FVM_OK : FVM_ERROR)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_drag_target(c, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile && profile + 3 < 7)
        {
            if (!(out.type == FVM_VALUE_OBJECT && out.object == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_exec_drag_end(c) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
