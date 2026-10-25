/* Host point methods preserve overflow propagation and inverse-then-apply cancellation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <float.h>


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned native, reverse;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (native = 0; native < 2; ++native)
    {
        for (reverse = 0; reverse < 2; ++reverse)
        {
            FVM_Context *c;
            FVM_Object *clip, *point;
            FVM_Value v, arg, out;
            FVM_Affine m = {2, 0, 0, 1, 0, 0};
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
                  fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK &&
                  fvm_object_create(c, &point) == FVM_OK && fvm_gc_pin(c, point) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (reverse)
            {
                m.xx = 3;
                m.x0 = 1e16;
            }
            if (!(fvm_movieclip_set_matrix(c, clip, &m) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, reverse ? 1e16 + 2 : DBL_MAX);
            if (!(fvm_object_set(c, point, "x", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 0);
            if (!(fvm_object_set(c, point, "y", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&arg);
            arg.type = FVM_VALUE_OBJECT;
            arg.object = point;
            if (!((native ? fvm_object_call(c, clip, reverse ? "globalToLocal" : "localToGlobal", 1, &arg, &out)
                          : fvm_exec_clip_point(c, clip, reverse, &arg)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, point, "x", &out) == FVM_OK && (reverse ? out.number == 1 : out.number > DBL_MAX)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, point, "y", &out) == FVM_OK && out.number == 0 && fvm_gc_collect(c, 0) == FVM_OK))
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
