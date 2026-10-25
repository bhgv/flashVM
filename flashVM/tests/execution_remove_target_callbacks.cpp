/* Explicit target removal retains collecting/throwing path operands without changing activation. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned conversions;
static bool throw_path;


static FVM_Result path_text(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    if (!(fvm_set_global(c, "input", &v) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
          fvm_object_get(c, frame->receiver, "alive", &v) == FVM_OK && v.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++conversions;
    if (throw_path)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    return fvm_value_string(out, "victim");
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned fail = 0; fail < 2; ++fail)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0, *input = 0;
            FVM_Value args[2], out, value;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "victim");
            fvm_value_number(args + 1, 2);
            if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = out.object;
            if (!(fvm_gc_pin(c, clip) == FVM_OK && fvm_object_create(c, &input) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 17);
            if (!(fvm_object_set(c, input, "alive", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            attach(c, input, "toString", path_text, 0);
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = input;
            if (!(fvm_set_global(c, "input", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            conversions = 0;
            throw_path = fail != 0;
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Result r = fvm_exec_remove_target(c, root, &value);
            if (!(conversions == 1 && c->gc_frames == roots && !c->avm1_target_active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (fail)
            {
                if (!(r == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_exec_clip_detached(c, clip, &out) == FVM_OK && !!out.boolean == !fail))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
