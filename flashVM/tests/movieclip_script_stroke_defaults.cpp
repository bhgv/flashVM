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
    FVM_Object *data = clip->resource_result.object;
    FVM_Value a[8], out;
    const double widths[] = {-10, 0, .25, 255, 256, 1e100};
    const double expected[] = {0, 0, .25, 255, 255, 255};
    for (unsigned i = 0; i < 6; ++i)
    {
        ss_args(a, 2, 1, widths[i]);
        if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(data->properties["width"].number == expected[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(a, 4);
    fvm_value_undefined(a + 1);
    fvm_value_undefined(a + 2);
    if (!(fvm_object_call(c, clip, "lineStyle", 3, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(data->properties["lineRgb"].number == 0 && data->properties["lineAlpha"].number == 100 &&
          data->properties.find("lineCap") == data->properties.end() &&
          data->properties.find("lineJoin") == data->properties.end()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(a);
    if (!(fvm_object_call(c, clip, "lineStyle", 8, a, &out) == FVM_OK && data->properties["stroke"].number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_style(c, clip, 2, 1);
    if (!(fvm_object_call(c, clip, "lineStyle", 0, 0, &out) == FVM_OK && data->properties["stroke"].number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ss_metadata(clip, 2, 1);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
