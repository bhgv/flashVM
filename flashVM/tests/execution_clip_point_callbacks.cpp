/* Coordinate getters run before geometry; setters preserve x-then-y errors and native barriers. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"

static unsigned phase, fail;
static double written[2];


static FVM_Result read_x(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(phase++ == 0 && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}


static FVM_Result read_y(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(phase++ == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 2);
    return FVM_OK;
}


static FVM_Result set_x(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    if (!(phase++ == 2 && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    written[0] = a[0].number;
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result set_y(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    if (!(phase++ == 3 && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    written[1] = a[0].number;
    fvm_value_undefined(out);
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
        for (fail = 0; fail < 2; ++fail)
        {
            FVM_Context *c = 0;
            FVM_Object *clip = 0, *point = 0;
            FVM_Value v, out;
            FVM_Affine matrix = {2, 0, 0, 3, 10, 20};
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
                  fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK &&
                  fvm_movieclip_set_matrix(c, clip, &matrix) == FVM_OK && fvm_object_create(c, &point) == FVM_OK &&
                  fvm_gc_pin(c, point) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_accessor_rooted(c, point, "x", read_x, set_x, 0, 0) == FVM_OK &&
                  fvm_object_define_accessor_rooted(c, point, "y", read_y, set_y, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = point;
            phase = 0;
            written[0] = written[1] = 0;
            FVM_GCRootFrame *roots = c->gc_frames;
            FVM_Result r =
                native ? fvm_object_call(c, clip, "localToGlobal", 1, &v, &out) : fvm_exec_clip_point(c, clip, 0, &v);
            if (!(phase == 4 && written[0] == 12 && written[1] == (fail ? 0 : 26) && c->gc_frames == roots &&
                  !c->gc_native_depth && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(r == (fail ? FVM_EXCEPTION : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (fail)
            {
                if (!(fvm_exception_clear(c) == FVM_OK))
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
    fvm_runtime_destroy(rt);
    return 0;
}
