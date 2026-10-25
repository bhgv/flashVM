/* Linear-light records preserve both aliases, copied inputs and receiver independence after sender teardown. */
#include <stdio.h>
#include <stdlib.h>
#include "gradient_linear_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned stroke = 0; stroke < 2; ++stroke)
    {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            for (unsigned radial = 0; radial < 2; ++radial)
            {
                FVM_Context *c = 0, *d = 0, *e = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &d) == FVM_OK &&
                      fvm_context_create(rt, &e) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *root = 0;
                if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *clip = new_child(c, root, "source", 1);
                FVM_Value args[8], out;
                linear_arguments(c, args, mode, radial != 0);
                if (stroke)
                {
                    FVM_Value width;
                    fvm_value_number(&width, 4);
                    if (!(fvm_object_call(c, clip, "lineStyle", 1, &width, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                const char *method = stroke ? "lineGradientStyle" : "beginGradientFill",
                           *key = stroke ? "lineGradient" : "gradient";
                if (!(fvm_object_call(c, clip, method, 8, args, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (stroke)
                {
                    line_gradient_geometry(c, clip);
                }
                else
                {
                    geometry(c, clip, false);
                }
                FVM_Object *record = clip->resource_result.object->properties[key].object;
                FVM_Value duplicate[2];
                if (!(fvm_value_string(duplicate, "copy") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(duplicate + 1, 2);
                if (!(fvm_object_call(c, clip, "duplicateMovieClip", 2, duplicate, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *copy = out.object, *data = copy->resource_result.object,
                           *cloned = data->properties[key].object;
                if (!(cloned != record && cloned->properties["interpolation"].number == 1 &&
                      data->properties["active"].object->properties[key].object == cloned))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                set(c, args[4].object, "w", 900);
                FVM_Value changed;
                fvm_value_number(&changed, 0);
                if (!(fvm_table_set_index(c, args[1].object, 0, &changed) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                linear_pixels(c, clip, mode, radial != 0, stroke != 0);
                linear_pixels(c, copy, mode, radial != 0, stroke != 0);
                FVM_Value value;
                ref(&value, copy);
                unsigned char *wire = 0;
                size_t size = 0;
                if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(d, wire, size, &out) == FVM_OK && fvm_gc_pin(d, out.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                FVM_Object *binary = out.object;
                FVM_Channel *channel = 0;
                if (!(fvm_channel_create(&channel) == FVM_OK &&
                      fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
                if (!(fvm_channel_receive_value(e, channel, &out, 0) == FVM_OK && fvm_gc_pin(e, out.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_channel_destroy(channel);
                FVM_Object *received = out.object;
                if (!(fvm_gc_collect(d, 0) == FVM_OK && fvm_gc_collect(e, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                linear_pixels(d, binary, mode, radial != 0, stroke != 0);
                linear_pixels(e, received, mode, radial != 0, stroke != 0);
                binary->resource_result.object->properties[key].object->properties.erase("interpolation");
                linear_pixels(d, binary, mode, radial != 0, stroke != 0, 0);
                linear_pixels(e, received, mode, radial != 0, stroke != 0);
                if (!(!d->gc_frames && !d->gc_native_depth && !e->gc_frames && !e->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(d);
                fvm_context_destroy(e);
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
