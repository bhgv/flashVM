/* Loaded point conversion uses own numeric fields and suppresses getters/coercion/invalid args. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"

static unsigned calls;


static FVM_Result trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    fvm_value_number(out, 99);
    return FVM_OK;
}


static void set(FVM_Context *c, FVM_Object *o, const char *key, double number)
{
    FVM_Value v;
    fvm_value_number(&v, number);
    if (!(fvm_object_set(c, o, key, &v) == FVM_OK))
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
        for (unsigned native = 0; native < 2; ++native)
        {
            for (unsigned mode = 0; mode < 7; ++mode)
            {
                FVM_Context *c = 0;
                FVM_Object *clip = 0, *point = 0, *proto = 0;
                FVM_Value value, out;
                FVM_Affine matrix = {2, 0, 0, 3, 10, 20};
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                      fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK &&
                      fvm_gc_pin(c, clip) == FVM_OK && fvm_movieclip_set_matrix(c, clip, &matrix) == FVM_OK &&
                      fvm_object_create(c, &point) == FVM_OK && fvm_gc_pin(c, point) == FVM_OK &&
                      fvm_object_create(c, &proto) == FVM_OK && fvm_gc_pin(c, proto) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                set(c, point, "y", 2);
                if (mode == 0)
                {
                    set(c, point, "x", 1);
                }
                if (mode == 1)
                {
                    set(c, proto, "x", 1);
                    point->prototype = proto;
                }
                if (mode == 2)
                {
                    if (!(fvm_object_define_accessor_rooted(c, point, "x", trap, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (mode == 3)
                {
                    fvm_value_string(&value, "1");
                    if (!(fvm_object_set(c, point, "x", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (mode == 4)
                {
                    FVM_NativeEntry method;
                    method.callback = trap;
                    proto->methods["valueOf"] = method;
                    fvm_value_undefined(&value);
                    value.type = FVM_VALUE_OBJECT;
                    value.object = proto;
                    if (!(fvm_object_set(c, point, "x", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_value_undefined(&value);
                if (mode < 5)
                {
                    value.type = FVM_VALUE_OBJECT;
                    value.object = point;
                }
                else if (mode == 5)
                {
                    fvm_value_number(&value, 1);
                }
                if (mode)
                {
                    int deleted = 0;
                    if (!(fvm_object_delete(c, clip, "_x", &deleted) == FVM_OK && deleted &&
                          fvm_object_define_accessor_rooted(c, clip, "_x", trap, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                calls = 0;
                FVM_GCRootFrame *roots = c->gc_frames;
                FVM_Result r =
                    native ? fvm_object_call(c, clip, "localToGlobal", mode == 6 ? 0 : 1, mode == 6 ? 0 : &value, &out)
                           : fvm_exec_clip_point(c, clip, 0, &value);
                if (!(r == FVM_OK && !calls && c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (native)
                {
                    if (!(out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_object_get(c, point, "y", &out) == FVM_OK && out.number == (mode == 0 ? 26 : 2)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (mode == 0)
                {
                    if (!(fvm_object_get(c, point, "x", &out) == FVM_OK && out.number == 12))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
