#include <stdio.h>
#include <stdlib.h>
#include "script_stroke_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_context(rt, &c);
    FVM_Object *clip = ss_clip(c, 2, 1);
    FVM_Object *data = clip->resource_result.object, *active = data->properties["active"].object;
    FVM_Value a[8], out;
    for (unsigned field = 3; field <= 6; ++field)
    {
        ss_args(a, 1, 0);
        if (field == 3)
        {
            fvm_value_boolean(a + field, 1);
        }
        else
        {
            if (!(fvm_value_string(a + field, field == 6 ? "miter" : "unsupported") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(data->properties["active"].object == active && data->properties["lineCap"].number == 2 &&
              data->properties["lineJoin"].number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const double invalid[] = {HUGE_VAL, HUGE_VAL - HUGE_VAL};
    for (unsigned i = 0; i < 2; ++i)
    {
        ss_args(a, 1, 0, invalid[i]);
        if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == FVM_INVALID_ARGUMENT &&
              data->properties["active"].object == active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const char embedded[] = {'s', 'q', 'u', 'a', 'r', 'e', 0, 'x'};
    ss_args(a, 1, 0);
    if (!(fvm_value_string_bytes(a + 5, embedded, sizeof(embedded)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_pixels(c, clip, 2, 1);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
