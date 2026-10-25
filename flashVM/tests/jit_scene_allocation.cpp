/* Native scene allocation failures restore roots and active code across mutation and dragging. */
#define main scene_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_SCENE_FIXTURE_ONLY
#include "jit_scene.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (unsigned kind = 0; kind < 4; ++kind)
        {
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 3000 && !success; ++boundary)
            {
                FVM_Context *c = 0;
                const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (loaded)
                {
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_Object *child = 0, *root = display_setup(c, &child);
                Program p;
                p.number(77);
                if (kind == 0)
                {
                    p.text("child");
                    p.text("copy");
                    p.number(16387);
                    p.op(0x24);
                }
                if (kind == 1)
                {
                    p.text("child");
                    p.op(0x25);
                }
                if (kind == 2)
                {
                    scene_drag(p, true);
                }
                if (kind == 3)
                {
                    if (!(fvm_exec_drag_begin(c, child, 1, 0) == FVM_OK && fvm_input_set_pointer(c, 8, 9) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    p.op(0x28);
                }
                p.finish();
                FVM_Object *fn = 0;
                FVM_JitCode *code = 0;
                FVM_Value out;
                if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
                      fvm_gc_pin(c, fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK &&
                      fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result status = fvm_function_invoke(c, fn, root, 0, 0, &out);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth &&
                      !c->drag_updating && !code->active))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OK)
                {
                    success = true;
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 77))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 0)
                    {
                        if (!(fvm_object_get(c, root, "copy", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
                              out.object != child))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 1)
                    {
                        if (!(fvm_exec_clip_detached(c, child, &out) == FVM_OK && out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 2)
                    {
                        if (!(c->drag_clip == child && c->drag_lock_center && c->drag_constrained))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 3)
                    {
                        if (!(!c->drag_clip))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        scene_position(c, child, 8, 9);
                    }
                }
                else
                {
                    if (!(status == FVM_OUT_OF_MEMORY))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
                if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK &&
                      fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_exec_drag_end(c) == FVM_OK))
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
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
