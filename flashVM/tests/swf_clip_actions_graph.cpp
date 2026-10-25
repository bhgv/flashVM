/* Placement handlers travel as owned graph bytes and retain source version after sender destruction. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_actions_fixture.hpp"


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
        FVM_Context *source = clip_context(rt), *dest = clip_context(rt);
        Bytes swf = clip_movie(version, clip_event_table(version, 3, action(42)));
        FVM_Object *root = clip_load(source, swf);
        if (!(fvm_movieclip_advance(source, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(source, root, "actor").object;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = actor;
        unsigned char *wire = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(source, &value, &wire, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(source);
        if (!(fvm_value_deserialize(dest, wire, size, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(wire);
        actor = value.object;
        if (!(fvm_gc_pin(dest, actor) == FVM_OK && fvm_gc_string_mode(dest, FVM_GC_STRINGS_TRACED) == FVM_OK &&
              fvm_gc_collect(dest, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_clip_actions(dest, actor, 1, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(dest, actor, "seen", 42);
        if (!(fvm_movieclip_dispatch_clip_actions(dest, actor, 0x40000, 0) == (version < 7 ? FVM_UNSUPPORTED : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(dest, actor, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip_clean(dest);
        fvm_context_destroy(dest);
    }
    fvm_runtime_destroy(rt);
}
