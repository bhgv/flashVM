/* Loaded detached/replaced clips retain AVM1 parent/root and absolute path provenance. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_state.hpp"


static FVM_Object *make_child(FVM_Context *c, FVM_Object *parent, const char *key, double depth)
{
    FVM_Value a[2], out;
    fvm_value_string(a, key);
    fvm_value_number(a + 1, depth);
    if (!(fvm_object_call(c, parent, "createEmptyMovieClip", 2, a, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out.object;
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
    for (unsigned profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c = 0;
        FVM_Object *root = 0;
        FVM_Value out, path;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)(profile + 3);
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *old = make_child(c, root, "old", 1), *nested = make_child(c, old, "nested", 0);
        if (!(fvm_gc_pin(c, old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *replacement = make_child(c, root, "replacement", 1);
        if (!(old->resource_result.object->properties["parent"].type == FVM_VALUE_NULL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_preload_display(c, old, true, &out) == FVM_OK &&
              (profile ? out.object == root : out.type == FVM_VALUE_NULL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_preload_display(c, nested, false, &out) == FVM_OK && out.object == (profile ? root : old)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_detach_clip(c, old) == FVM_OK && fvm_exec_detach_clip(c, old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_preload_display(c, old, true, &out) == FVM_OK &&
              (profile ? out.object == root : out.type == FVM_VALUE_NULL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&path, "/replacement");
        FVM_Object *resolved = 0;
        if (!(fvm_movieclip_resolve_path(c, nested, path, &resolved) == FVM_OK &&
              resolved == (profile ? replacement : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        { /* A removed base has no selected target: unqualified lookup falls back to its retained root. */
            fvm_value_number(&out, 73);
            if (!(fvm_object_set(c, root, "marker", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *fn = 0;
            /* Explicitly selecting an invalid target exercises the same target-or-root fallback. */
            Bytes body;
            text(body, "absent");
            body.push_back(0x20);
            text(body, "marker");
            body.push_back(0x1c);
            end(body);
            if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK &&
                  fvm_function_invoke(c, fn, old, 0, 0, &out) == FVM_OK && out.number == 73))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_gc_unpin(c, root) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_preload_display(c, old, false, &out) == FVM_OK && out.object == (profile ? root : old)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Context *other = 0;
        if (!(fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Channel *channel = 0;
        FVM_Value packet;
        fvm_value_undefined(&packet);
        packet.type = FVM_VALUE_OBJECT;
        packet.object = old;
        if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &packet, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &packet, 0) == FVM_OK &&
              fvm_gc_pin(other, packet.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *copy = packet.object;
        if (!(fvm_movieclip_preload_display(other, copy, false, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            if (!(out.object != copy && out.object != root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_resolve_path(other, copy, path, &resolved) == FVM_OK && resolved &&
                  resolved->owner == other))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.object == copy))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_channel_destroy(channel);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
