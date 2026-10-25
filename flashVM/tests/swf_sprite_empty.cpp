/* Zero-frame sprite definitions retain their metadata through GC and transfer. */
#define main sprite_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "swf_sprite_resources.cpp"

#undef main
#include "../src/core/runtime.hpp"


static void inspect(FVM_Object *root, unsigned declared)
{
    FVM_Object *library = root->resource_result.object->properties["library"].object;
    FVM_Object *definition = library->properties["sprites"].object->indexed[7].object;
    if (!(definition && definition->properties["total"].number == declared && definition->properties["id"].number == 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *frames = definition->properties["frames"].object;
    if (!(frames && frames->indexed.size() == declared))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(library->properties["exports"].object->properties["ClassSymbol"].object == definition))
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned declared = 0; declared <= 3; ++declared)
        {
            FVM_Context *c = 0, *other = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes empty, controls, tags;
            tag(controls, 0, empty);
            tag(controls, 0, empty);
            tag(tags, 39, sprite(7, declared, controls));
            export_asset(tags, 7, "ClassSymbol");
            tag(tags, 12, action(19));
            tag(tags, 1, empty);
            tag(tags, 0, empty);
            Bytes swf = movie(tags, 1);
            swf[3] = (unsigned char)version;
            FVM_Object *root = 0;
            if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, root) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            inspect(root, declared);
            if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, root, "seen", 19);
            inspect(root, declared);
            FVM_Channel *channel = 0;
            if (!(fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value, out;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = root;
            if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.type == FVM_VALUE_OBJECT && fvm_gc_pin(other, out.object) == FVM_OK &&
                  fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            inspect(out.object, declared);
            if (!(fvm_movieclip_advance(other, out.object, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(other, out.object, "seen", 19);
            Bytes bad_controls, bad;
            tag(bad_controls, 1, empty);
            tag(bad_controls, 0, empty);
            tag(bad, 39, sprite(7, 0, bad_controls));
            tag(bad, 1, empty);
            tag(bad, 0, empty);
            rejects(c, bad, FVM_INVALID_ARGUMENT);
            bad_controls = controls;
            bad_controls.push_back(1);
            bad.clear();
            tag(bad, 39, sprite(7, 0, bad_controls));
            tag(bad, 1, empty);
            tag(bad, 0, empty);
            rejects(c, bad, FVM_INVALID_ARGUMENT);
            bad.clear();
            tag(bad, 39, sprite(7, 0, empty));
            tag(bad, 1, empty);
            tag(bad, 0, empty);
            rejects(c, bad, FVM_INVALID_ARGUMENT);
            bad.clear();
            tag(bad, 39, sprite(7, 0, controls));
            tag(bad, 39, sprite(7, 0, controls));
            tag(bad, 1, empty);
            tag(bad, 0, empty);
            rejects(c, bad, FVM_INVALID_ARGUMENT);
            fvm_channel_destroy(channel);
            fvm_context_destroy(other);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
