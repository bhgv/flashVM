/* Numeric coercion precedes loaded hit geometry; moved roots and exceptions preserve call scopes. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned conversions;
static bool target_failing, collecting;


static FVM_Result target_path(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++conversions;
    if (collecting)
    {
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (target_failing)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    return fvm_value_string(out, "target");
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
        FVM_Context *c = 0;
        FVM_Object *root = 0, *input = 0;
        FVM_Value a[2], out;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(a, "target");
        fvm_value_number(a + 1, 0);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = out.object;
        if (!(fvm_object_create(c, &input) == FVM_OK && fvm_gc_pin(c, input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        attach(c, input, "toString", target_path, 0);
        for (unsigned native = 0; native < 2; ++native)
        {
            for (unsigned fail = 0; fail < 2; ++fail)
            {
                fvm_value_undefined(a);
                a->type = FVM_VALUE_OBJECT;
                a->object = input;
                fvm_value_number(&out, 73);
                conversions = 0;
                target_failing = fail != 0;
                collecting = !native;
                FVM_GCRootFrame *roots = c->gc_frames;
                FVM_Result r =
                    native ? fvm_object_call(c, root, "hitTest", 1, a, &out) : fvm_exec_resolve_clip(c, root, a, 0, a);
                if (!(conversions == 1 && c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (fail)
                {
                    if (!(r == FVM_EXCEPTION && a->object == input && fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (native)
                {
                    if (!(r == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(r == FVM_OK && a->type == FVM_VALUE_OBJECT && a->object == clip))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
