/* Key/Mouse/Selection registry work is transactional across native and real AVM1 routes. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "../src/core/execution_work_scope.hpp"


static fvm_detail::vector<FVM_Object *> &registry(FVM_Context *c, unsigned kind)
{
    return kind == 0 ? c->keyboard_listeners : kind == 1 ? c->mouse_listeners : c->selection_listeners;
}


static FVM_Result registration(FVM_Context *c,
                               FVM_Object *ns,
                               FVM_Object *listener,
                               unsigned kind,
                               unsigned route,
                               bool remove,
                               FVM_Result expected,
                               int *removed)
{
    if (!route && kind != 2)
    {
        FVM_Result result =
            remove ? (kind ? fvm_input_remove_mouse_listener(c, listener, removed)
                           : fvm_input_remove_key_listener(c, listener, removed))
                   : (kind ? fvm_input_add_mouse_listener(c, listener) : fvm_input_add_key_listener(c, listener));
        if (!(result == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return result;
    }
    FVM_Value args = class_value(listener), out;
    const char *name = remove ? "removeListener" : "addListener";
    if (route < 2)
    {
        if (route)
        {
            FVM_Value fn;
            if (!(fvm_object_get(c, ns, name, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_invoke(c, fn.object, ns, 1, &args, &out) == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_object_call(c, ns, name, 1, &args, &out) == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        Program p;
        p.variable("candidate");
        p.number(1);
        p.variable("inputNamespace");
        p.text(name);
        p.op(0x52);
        p.finish();
        class_mode = route - 2;
        out = class_run(c, p, expected);
    }
    return expected;
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
    const unsigned long ordinary = rt->config.instruction_limit;
    const char *names[] = {"Key", "Mouse", "Selection"};
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 3; ++kind)
        {
            for (unsigned route = 0; route < (fvm_jit_available() ? 5U : 3U); ++route)
            {
                for (unsigned action = 0; action < 3; ++action)
                {
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_standard_keyboard_install(c) == FVM_OK && fvm_standard_mouse_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_standard_selection_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value ns;
                    if (!(fvm_get_global(c, names[kind], &ns) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_set_global(c, "inputNamespace", &ns) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *first = 0, *candidate = 0;
                    for (unsigned i = 0; i < 80; ++i)
                    {
                        FVM_Object *o = 0;
                        if (!(fvm_object_create(c, &o) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        int removed = 0;
                        registration(c, ns.object, o, kind, 0, false, FVM_OK, &removed);
                        if (!i)
                        {
                            first = o;
                        }
                        candidate = o;
                    }
                    if (!action)
                    {
                        if (!(fvm_object_create(c, &candidate) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (action == 1)
                    {
                        candidate = first;
                    }
                    FVM_Value arg = class_value(candidate);
                    if (!(fvm_set_global(c, "candidate", &arg) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_detail::vector<FVM_Object *> before = registry(c, kind);
                    rt->config.instruction_limit = action == 0 ? 100 : 32;
                    int removed = 77;
                    registration(c, ns.object, candidate, kind, route, action == 1, FVM_ERROR, &removed);
                    if (!(removed == 77 && registry(c, kind).size() == before.size()))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (size_t i = 0; i < before.size(); ++i)
                    {
                        if (!(registry(c, kind)[i] == before[i]))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth && !c->script_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    /* Failed inspection must not expose a half-filled Array or change native roots. */
                    rt->config.instruction_limit = 2;
                    FVM_Value inspection;
                    if (!(fvm_object_get(c, ns.object, "_listeners", &inspection) == FVM_ERROR))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(registry(c, kind).size() == 80))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    rt->config.instruction_limit = ordinary;
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    registration(c, ns.object, candidate, kind, route, action == 1, FVM_OK, &removed);
                    if (!(registry(c, kind).size() == (action == 0 ? 81U : action == 1 ? 79U : 80U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    rt->config.instruction_limit = 3;
                    {


                        FVM_ExecutionWorkScope outer(c);
                        if (!(outer.step() == FVM_OK && outer.step() == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        registration(c, ns.object, candidate, kind, 0, false, FVM_ERROR, &removed);
                        if (!(c->avm1_steps == 3 && c->avm1_execution_depth == 1))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    rt->config.instruction_limit = ordinary;
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
