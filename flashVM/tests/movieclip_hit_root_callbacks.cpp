/* Numeric coercion precedes loaded hit geometry; moved roots and exceptions preserve call scopes. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static FVM_Object *expected_root;
static unsigned conversions;
static bool fail_number;


static FVM_Result move_root(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++conversions;
    if (fail_number)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    FVM_Value v;
    fvm_value_number(&v, 1000);
    if (!(fvm_object_set(c, expected_root, "_x", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 7);
    return FVM_OK;
}


static void coordinate(FVM_Context *c, FVM_Object *clip, const char *method, double x, double y)
{
    FVM_Value a[2], out;
    fvm_value_number(a, x);
    fvm_value_number(a + 1, y);
    if (!(fvm_object_call(c, clip, method, 2, a, &out) == FVM_OK))
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        FVM_Object *root = 0, *input = 0;
        FVM_Value a[3], out, v;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expected_root = root;
        fvm_value_string(a, "triangle");
        fvm_value_number(a + 1, 0);
        if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = out.object;
        fvm_value_number(&v, 5);
        if (!(fvm_object_set(c, clip, "_x", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, clip, "_lockroot", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(a, 0xff0000);
        if (!(fvm_object_call(c, clip, "beginFill", 1, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        coordinate(c, clip, "moveTo", 0, 0);
        coordinate(c, clip, "lineTo", 10, 0);
        coordinate(c, clip, "lineTo", 0, 10);
        if (!(fvm_object_call(c, clip, "endFill", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &input) == FVM_OK && fvm_gc_pin(c, input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        attach(c, input, "valueOf", move_root, 0);
        for (unsigned failing = 0; failing < 2; ++failing)
        {
            for (unsigned shape = 0; shape < 2; ++shape)
            {
                fvm_value_number(&v, 0);
                if (!(fvm_object_set(c, root, "_x", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_undefined(a);
                a[0].type = FVM_VALUE_OBJECT;
                a[0].object = input;
                fvm_value_number(a + 1, 2);
                fvm_value_boolean(a + 2, shape);
                conversions = 0;
                fail_number = failing != 0;
                FVM_GCRootFrame *roots = c->gc_frames;
                FVM_Result r = fvm_object_call(c, clip, "hitTest", 3, a, &out);
                if (!(conversions == 1 && c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (failing)
                {
                    if (!(r == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(r == FVM_OK && out.type == FVM_VALUE_BOOLEAN && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
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
