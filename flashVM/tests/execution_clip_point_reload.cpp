/* Loading a SWF inside a geometry getter cannot switch to an uninitialized matrix representation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cmath>

static unsigned calls;


static FVM_Result load(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    unsigned char swf[] = {'F', 'W', 'S', 4, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    ++calls;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
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
    for (unsigned native = 0; native < 2; ++native)
    {
        FVM_Context *c = 0;
        FVM_Object *clip = 0, *point = 0;
        FVM_Value v, out;
        FVM_Affine matrix = {0.5, 0, 0, 0.5, 1, 2};
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK &&
              fvm_movieclip_set_matrix(c, clip, &matrix) == FVM_OK && fvm_object_create(c, &point) == FVM_OK &&
              fvm_gc_pin(c, point) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 0.05);
        if (!(fvm_object_set(c, point, "x", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 0.15);
        if (!(fvm_object_set(c, point, "y", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int deleted = 0;
        if (!(fvm_object_delete(c, clip, "_x", &deleted) == FVM_OK && deleted &&
              fvm_object_define_accessor_rooted(c, clip, "_x", load, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = point;
        FVM_GCRootFrame *roots = c->gc_frames;
        calls = 0;
        if (!((native ? fvm_object_call(c, clip, "localToGlobal", 1, &v, &out) : fvm_exec_clip_point(c, clip, 0, &v)) ==
                  FVM_OK &&
              calls == 1 && c->swf_loaded))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, point, "x", &out) == FVM_OK && std::fabs(out.number - 1.025) < 1e-9 &&
              fvm_object_get(c, point, "y", &out) == FVM_OK && std::fabs(out.number - 2.075) < 1e-9))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
