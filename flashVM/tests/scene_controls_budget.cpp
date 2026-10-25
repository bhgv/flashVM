/* Exact scene preparation/application costs and nested work keep the caller's ceiling. */
#include <stdio.h>
#include <stdlib.h>
#include "scene_controls_fixture.hpp"
#include "../src/standard/movieclip_scene.hpp"
#include <limits>


static void exact(FVM_Runtime *rt, bool reconstruct, unsigned long limit)
{
    rt->config.instruction_limit = 0;
    FVM_Context *c = tick_context(rt);
    SceneControlProbe p;
    FVM_Object *root = scene_control_root(c, p);
    rt->config.instruction_limit = limit;
    unsigned long required = reconstruct ? 11 : 6;
    FVM_Result result = fvm_movieclip_scene_run(c, root, reconstruct ? 2 : 1, reconstruct);
    if (!(result == (limit && limit < required ? FVM_ERROR : FVM_OK)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->avm1_steps == (limit && limit < required ? limit : required)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    if (reconstruct && limit && limit < 9)
    {
        if (!(p.calls == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!reconstruct && limit && limit <= 3)
    {
        if (!(p.calls == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!limit || limit >= required)
    {
        if (!(p.calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
}


struct NestedScene
{
    FVM_Object *other;
    unsigned calls;
};


static FVM_Result nested(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    NestedScene *p = static_cast<NestedScene *>(raw);
    ++p->calls;
    FVM_Result result = p->calls == 1 ? fvm_movieclip_scene_run(c, p->other, 1, false) : FVM_OK;
    fvm_value_undefined(out);
    return result;
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
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        for (unsigned long limit = 0; limit <= 12; ++limit)
        {
            exact(rt, mode != 0, limit);
        }
    }
    /* Reconstruction also accounts for copying and checking existing child identities. */
    for (unsigned long limit = 0; limit <= 18; ++limit)
    {
        rt->config.instruction_limit = 0;
        FVM_Context *c = tick_context(rt);
        SceneControlProbe p;
        FVM_Object *root = scene_control_root(c, p);
        if (!(fvm_movieclip_scene_run(c, root, 1, false) == FVM_OK && p.calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = limit;
        FVM_Result result = fvm_movieclip_scene_run(c, root, 2, true);
        if (!(result == (limit && limit < 17 ? FVM_ERROR : FVM_OK) && p.calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == (limit && limit < 17 ? limit : 17)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
    }
    /* Empty authored gaps cannot bypass native work just because there are no actions. */
    rt->config.instruction_limit = 0;
    FVM_Context *gap = tick_context(rt);
    FVM_Object *empty = tick_root(gap, 65535);
    rt->config.instruction_limit = 10;
    if (!(fvm_movieclip_scene_run(gap, empty, 65535, true) == FVM_ERROR && gap->avm1_steps == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(gap);
    fvm_context_destroy(gap);
    rt->config.instruction_limit = 0;
    FVM_Context *c = tick_context(rt);
    SceneControlProbe p;
    FVM_Object *root = scene_control_root(c, p);
    FVM_Object *other = tick_root(c);
    FVM_Object *frames = other->resource_result.object->properties["frames"].object, *frame = 0, *list = 0,
               *control = 0;
    if (!(fvm_object_create(c, &frame) == FVM_OK && fvm_object_create(c, &list) == FVM_OK &&
          fvm_object_create(c, &control) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 9);
    control->properties["op"] = value;
    fvm_value_number(&value, 0);
    control->properties["background"] = value;
    list->indexed[0] = tick_object(control);
    list->indexed[1] = tick_object(control);
    frame->properties["controls"] = tick_object(list);
    frames->indexed[1] = tick_object(frame);
    NestedScene state = {other, 0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_native(c, nested, &state, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value klass, mc, args[2], out;
    if (!(fvm_get_global(c, "MovieClip", &mc) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = get(c, mc.object, "prototype");
    if (!(fvm_object_set(c, fn, "prototype", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(args, "Hero") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    args[1] = tick_object(fn);
    if (!(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.instruction_limit = 9;
    if (!(fvm_movieclip_scene_run(c, root, 1, false) == FVM_ERROR && state.calls == 2 && c->avm1_steps == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    /* Saturated inherited execution work is neither wrapped nor reset by the scene scope. */
    c->avm1_execution_depth = 1;
    c->avm1_steps = std::numeric_limits<unsigned long>::max();
    if (!(fvm_movieclip_scene_run(c, root, 2, true) == FVM_ERROR &&
          c->avm1_steps == std::numeric_limits<unsigned long>::max() && c->avm1_execution_depth == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->avm1_execution_depth = 0;
    tick_clean(c);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
