#include <stdio.h>
#include <stdlib.h>
#include "script_stroke_code_fixture.hpp"


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
    Bytes tags, empty, a = ss_code(1, 1), b = ss_code(2, 1);
    tag(tags, 12, a);
    tag(tags, 1, empty);
    tag(tags, 12, b);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    FVM_Object *timeline = load(c, tags, 2), *clip = 0;
    if (!(fvm_movieclip_create(c, "paint", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    ref(&value, clip);
    if (!(fvm_set_global(c, "paintClip", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick(c, timeline);
    ss_metadata(clip, 1, 1);
    ss_pixels(c, clip, 1, 1);
    tick(c, timeline);
    FVM_Object *data = clip->resource_result.object, *paths = data->properties["paths"].object;
    if (!(paths->indexed.size() == 2 && paths->indexed[0].object->properties["lineCap"].number == 1 &&
          paths->indexed[1].object->properties["lineCap"].number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 16; ++i)
    {
        FVM_Value out;
        if (!(fvm_object_call(c, clip, "clear", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned frame = i % 2 + 1;
        seek(c, timeline, frame);
        ss_metadata(clip, frame == 1 ? 1 : 2, 1);
        ss_pixels(c, clip, frame == 1 ? 1 : 2, 1);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
