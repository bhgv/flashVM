/* Reject foreign private tables before legacy migration or any old-child removal. */
#include <stdio.h>
#include <stdlib.h>
#include "scene_controls_fixture.hpp"
#include "../src/standard/movieclip_scene.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned kind = 0; kind < 4; ++kind)
    {
        FVM_Context *c = tick_context(rt), *other = tick_context(rt);
        SceneControlProbe p;
        FVM_Object *root = scene_control_root(c, p);
        FVM_Object *foreign = tick_root(other), *state = root->resource_result.object;
        FVM_Object *foreign_state = foreign->resource_result.object,
                   *foreign_children = foreign_state->properties["children"].object;
        tick_child(other, foreign, "retained", 1);
        foreign_state->properties.erase("slot_bias");
        const size_t count = foreign_children->indexed.size();
        const unsigned long key = foreign_children->indexed.begin()->first;
        if (kind == 0)
        {
            root->resource_result = tick_object(foreign_state);
        }
        if (kind == 1)
        {
            state->properties["children"] = tick_object(foreign_children);
        }
        if (kind == 2)
        {
            state->properties["frames"] = tick_object(foreign_state->properties["frames"].object);
        }
        if (kind == 3)
        {
            state->properties["frames"].object->indexed[1] = tick_object(foreign_state);
        }
        FVM_Object *out = root;
        if (kind < 2)
        {
            if (!(fvm_movieclip_scene_children(root, &out) == FVM_INVALID_ARGUMENT && out == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_movieclip_scene_run(c, root, 1, false) == FVM_INVALID_ARGUMENT && !p.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(foreign_children->indexed.size() == count && foreign_children->indexed.begin()->first == key))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(foreign_state->properties.find("slot_bias") == foreign_state->properties.end()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
        fvm_context_destroy(other);
    }
    for (unsigned kind = 0; kind < 6; ++kind)
    {
        FVM_Context *c = tick_context(rt), *other = tick_context(rt);
        SceneControlProbe p;
        FVM_Object *root = scene_control_root(c, p);
        if (!(fvm_movieclip_scene_run(c, root, 1, false) == FVM_OK && p.calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *data = root->resource_result.object, *children = data->properties["children"].object;
        FVM_Object *first = get(c, root, "first").object;
        FVM_Value parent_before = get(c, first, "_parent");
        /* An empty desired scene would normally detach all three old instances. */
        data->properties["frames"].object->indexed[1].object->properties.erase("controls");
        FVM_Object *bad = 0;
        if (kind == 0)
        {
            children->indexed[999] = tick_object(first);
        }
        if (kind == 1)
        {
            children->indexed[999] = tick_object(root);
        }
        if (kind == 2)
        {
            if (!(fvm_object_create(c, &bad) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            children->indexed[999] = tick_object(bad);
        }
        if (kind == 3)
        {
            bad = tick_root(other);
            children->indexed[999] = tick_object(bad);
        }
        if (kind == 4)
        {
            FVM_Value value;
            fvm_value_number(&value, 7);
            children->indexed[999] = value;
        }
        if (kind == 5)
        {
            bad = tick_root(c);
            bad->resource_result = tick_object(tick_root(other)->resource_result.object);
            children->indexed[999] = tick_object(bad);
        }
        if (!(fvm_movieclip_scene_run(c, root, 2, true) == FVM_INVALID_ARGUMENT && p.calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get(c, root, "first").object == first && get(c, first, "_parent").object == parent_before.object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
        fvm_context_destroy(other);
    }
    fvm_runtime_destroy(rt);
}
