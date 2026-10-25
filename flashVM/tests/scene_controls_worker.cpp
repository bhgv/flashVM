/* Real workers observe cancellation while applying native-only authored controls. */
#include <stdio.h>
#include <stdlib.h>
#include "scene_controls_fixture.hpp"
#include "flashvm/worker.h"


struct SceneStop
{
    unsigned calls;
};


static FVM_Result stop_scene(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    SceneStop *p = static_cast<SceneStop *>(raw);
    ++p->calls;
    FVM_Worker *self = 0;
    if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result worker_scene(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SceneControlProbe p;
    FVM_Object *root = scene_control_root(c, p);
    SceneStop state = {0};
    FVM_Object *control = 0;
    if (!(fvm_object_create(c, &control) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 9);
    control->properties["op"] = value;
    fvm_value_number(&value, 0x112233);
    control->properties["background"] = value;
    for (unsigned i = 1; i < 200; ++i)
    {
        p.list->indexed[i] = tick_object(control);
    }
    FVM_Object *fn = 0;
    if (!(fvm_function_create_native(c, stop_scene, &state, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value klass, mc, args[2], ignored;
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
    if (!(fvm_object_call(c, klass.object, "registerClass", 2, args, &ignored) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!input->boolean)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    if (!(state.calls == (input->boolean ? 1U : 0U)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (input->boolean)
    {
        if (!(c->avm1_steps > 200 && c->avm1_steps < 400))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 0;
    for (unsigned i = 0; i < 2; ++i)
    {
        FVM_Value input, out;
        fvm_value_boolean(&input, i != 0);
        FVM_Worker *worker = 0;
        if (!(fvm_worker_run(&cfg, worker_scene, &input, 0, &worker) == FVM_OK &&
              fvm_worker_join(worker, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(worker);
    }
}
