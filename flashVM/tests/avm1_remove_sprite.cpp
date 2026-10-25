/* Native RemoveSprite dispatch, missing operands and current-target fallback. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static FVM_Object *base_expected, *target_expected;
static unsigned probes;


static FVM_Result inspect_target(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(c->avm1_base_clip == base_expected && c->avm1_target_clip == target_expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++probes;
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Object *child(FVM_Context *c, FVM_Object *root, const char *key, double depth)
{
    FVM_Value args[2], v;
    fvm_value_string(args, key);
    fvm_value_number(args + 1, depth);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return v.object;
}


static void run(FVM_Context *c, FVM_Object *root, Bytes &b)
{
    b.push_back(0);
    FVM_GCRootFrame *roots = c->gc_frames;
    if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), root) == FVM_OK && c->gc_frames == roots &&
          !c->avm1_target_active && !c->avm1_execution_depth && !c->script_depth))
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
        FVM_Value out, v;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)(profile + 3);
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
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
        for (unsigned negative = 0; negative < 2; ++negative)
        {
            for (unsigned direct = 0; direct < 2; ++direct)
            {
                FVM_Object *clip = child(c, root, "victim", negative ? -1 : 0);
                if (!(fvm_gc_pin(c, clip) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Bytes b;
                if (direct)
                {
                    fvm_value_undefined(&v);
                    v.type = FVM_VALUE_OBJECT;
                    v.object = clip;
                    if (profile)
                    {
                        if (!(fvm_exec_global_register(c, 1, 1, &v, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        reg(b, 1);
                    }
                    else
                    {
                        if (!(fvm_set_global(c, "input", &v) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        text(b, "input");
                        b.push_back(0x1c);
                    }
                }
                else
                {
                    text(b, "victim");
                }
                b.push_back(0x25);
                run(c, root, b);
                if (!(fvm_exec_clip_detached(c, clip, &out) == FVM_OK && !!out.boolean == !negative))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (negative)
                {
                    if (!(fvm_exec_detach_clip(c, clip) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        const unsigned char empty[] = {0x25, 0};
        if (!profile)
        {
            if (!(fvm_avm1_execute_receiver(c, empty, sizeof(empty), root) == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            FVM_Object *named = child(c, root, "undefined", 2);
            if (!(fvm_gc_pin(c, named) == FVM_OK &&
                  fvm_avm1_execute_receiver(c, empty, sizeof(empty), root) == FVM_OK &&
                  fvm_exec_clip_detached(c, named, &out) == FVM_OK && !!out.boolean == (profile + 3 >= 7)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *branch = child(c, root, "branch", 3), *nested = child(c, branch, "nested", 0);
        if (!(fvm_gc_pin(c, branch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        base_expected = root;
        target_expected = root;
        probes = 0;
        if (!(fvm_avm1_set_trace_rooted(c, inspect_target, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes b;
        b.push_back(0x8b);
        u16(b, 14);
        name(b, "branch/nested");
        text(b, "..");
        b.push_back(0x25);
        text(b, "probe");
        b.push_back(0x26);
        run(c, root, b);
        if (!(probes == 1 && fvm_exec_clip_detached(c, branch, &out) == FVM_OK && out.boolean &&
              fvm_exec_clip_detached(c, nested, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *self = child(c, root, "self", 4);
        if (!(fvm_gc_pin(c, self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        base_expected = self;
        target_expected = 0;
        probes = 0;
        b.clear();
        text(b, "");
        b.push_back(0x25);
        text(b, "probe");
        b.push_back(0x26);
        run(c, self, b);
        if (!(probes == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
