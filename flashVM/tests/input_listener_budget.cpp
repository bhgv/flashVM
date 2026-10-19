/* Key/Mouse/Selection registry work is transactional across native and real AVM1 routes. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "../src/core/execution_work_scope.hpp"

static fvm_detail::vector<FVM_Object *> &registry(FVM_Context *c, unsigned kind)
{ return kind == 0 ? c->keyboard_listeners : kind == 1 ? c->mouse_listeners : c->selection_listeners; }
static FVM_Result registration(FVM_Context *c, FVM_Object *ns, FVM_Object *listener,
    unsigned kind, unsigned route, bool remove, FVM_Result expected, int *removed)
{
    if (!route && kind != 2) {
        FVM_Result result = remove
            ? (kind ? fvm_input_remove_mouse_listener(c, listener, removed) : fvm_input_remove_key_listener(c, listener, removed))
            : (kind ? fvm_input_add_mouse_listener(c, listener) : fvm_input_add_key_listener(c, listener));
        assert(result == expected); return result;
    }
    FVM_Value args = class_value(listener), out;
    const char *name = remove ? "removeListener" : "addListener";
    if (route < 2) {
        if (route) {
            FVM_Value fn; assert(fvm_object_get(c, ns, name, &fn) == FVM_OK);
            assert(fvm_function_invoke(c, fn.object, ns, 1, &args, &out) == expected);
        } else assert(fvm_object_call(c, ns, name, 1, &args, &out) == expected);
    } else {
        Program p; p.variable("candidate"); p.number(1); p.variable("inputNamespace");
        p.text(name); p.op(0x52); p.finish(); class_mode = route - 2;
        out = class_run(c, p, expected);
    }
    return expected;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const unsigned long ordinary = rt->config.instruction_limit;
    const char *names[] = {"Key", "Mouse", "Selection"};
    for (unsigned version = 6; version <= 8; ++version)
    for (unsigned kind = 0; kind < 3; ++kind)
    for (unsigned route = 0; route < (fvm_jit_available() ? 5U : 3U); ++route)
    for (unsigned action = 0; action < 3; ++action) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        assert(fvm_standard_keyboard_install(c) == FVM_OK && fvm_standard_mouse_install(c) == FVM_OK);
        assert(fvm_standard_selection_install(c) == FVM_OK);
        FVM_Value ns; assert(fvm_get_global(c, names[kind], &ns) == FVM_OK);
        assert(fvm_set_global(c, "inputNamespace", &ns) == FVM_OK);
        FVM_Object *first = 0, *candidate = 0;
        for (unsigned i = 0; i < 80; ++i) {
            FVM_Object *o = 0; assert(fvm_object_create(c, &o) == FVM_OK);
            int removed = 0; registration(c, ns.object, o, kind, 0, false, FVM_OK, &removed);
            if (!i) first = o;
            candidate = o;
        }
        if (!action) assert(fvm_object_create(c, &candidate) == FVM_OK);
        if (action == 1) candidate = first;
        FVM_Value arg = class_value(candidate); assert(fvm_set_global(c, "candidate", &arg) == FVM_OK);
        fvm_detail::vector<FVM_Object *> before = registry(c, kind);
        rt->config.instruction_limit = action == 0 ? 100 : 32;
        int removed = 77;
        registration(c, ns.object, candidate, kind, route, action == 1, FVM_ERROR, &removed);
        assert(removed == 77 && registry(c, kind).size() == before.size());
        for (size_t i = 0; i < before.size(); ++i) assert(registry(c, kind)[i] == before[i]);
        assert(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth && !c->script_depth);
        /* Failed inspection must not expose a half-filled Array or change native roots. */
        rt->config.instruction_limit = 2;
        FVM_Value inspection; assert(fvm_object_get(c, ns.object, "_listeners", &inspection) == FVM_ERROR);
        assert(registry(c, kind).size() == 80);
        rt->config.instruction_limit = ordinary;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        registration(c, ns.object, candidate, kind, route, action == 1, FVM_OK, &removed);
        assert(registry(c, kind).size() == (action == 0 ? 81U : action == 1 ? 79U : 80U));
        rt->config.instruction_limit = 3;
        {
            FVM_ExecutionWorkScope outer(c);
            assert(outer.step() == FVM_OK && outer.step() == FVM_OK);
            registration(c, ns.object, candidate, kind, 0, false, FVM_ERROR, &removed);
            assert(c->avm1_steps == 3 && c->avm1_execution_depth == 1);
        }
        rt->config.instruction_limit = ordinary;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
