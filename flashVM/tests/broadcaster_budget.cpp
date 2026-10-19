/* Broadcaster registration work shares AVM1 limits and publishes only complete maps. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/broadcaster.h"
#include "../src/core/execution_work_scope.hpp"

static FVM_Value call_registration(FVM_Context *c, FVM_Object *source,
    FVM_Object *listener, const char *method, unsigned route, FVM_Result expected)
{
    FVM_Value arg = class_value(listener), out;
    if (!route)
        assert(fvm_object_call(c, source, method, 1, &arg, &out) == expected);
    else if (route == 1) {
        FVM_Value function;
        assert(fvm_object_get(c, source, method, &function) == FVM_OK);
        assert(fvm_function_invoke(c, function.object, source, 1, &arg, &out) == expected);
    } else {
        Program p;
        p.variable("candidate"); p.number(1); p.variable("source");
        p.text(method); p.op(0x52); p.finish();
        class_mode = route - 2;
        out = class_run(c, p, expected);
    }
    return out;
}

int main()
{
    FVM_Config cfg; FVM_Runtime *r = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK);
    const unsigned long ordinary_limit = r->config.instruction_limit;
    for (unsigned version = 6; version <= 8; ++version)
    for (unsigned route = 0; route < (fvm_jit_available() ? 5U : 3U); ++route)
    for (unsigned operation = 0; operation < 3; ++operation) {
        FVM_Context *c = 0; assert(fvm_context_create(r, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *source = 0, *candidate = 0;
        assert(fvm_object_create(c, &source) == FVM_OK);
        FVM_Value v = class_value(source);
        assert(fvm_set_global(c, "source", &v) == FVM_OK);
        assert(fvm_broadcaster_initialize(c, source) == FVM_OK);
        for (unsigned i = 0; i < 80; ++i) {
            FVM_Object *listener = 0; assert(fvm_object_create(c, &listener) == FVM_OK);
            v = class_value(listener); FVM_Value out;
            assert(fvm_object_call(c, source, "addListener", 1, &v, &out) == FVM_OK);
            if (i == (operation == 1 ? 0U : 79U)) candidate = listener;
        }
        if (!operation) assert(fvm_object_create(c, &candidate) == FVM_OK);
        v = class_value(candidate); assert(fvm_set_global(c, "candidate", &v) == FVM_OK);
        FVM_Value list; assert(fvm_object_get(c, source, "_listeners", &list) == FVM_OK);
        fvm_detail::map<unsigned long, FVM_Value> before = list.object->indexed;
        const char *method = operation == 1 ? "removeListener" : "addListener";
        /* Add fails during copying, remove during compaction, duplicate during search. */
        r->config.instruction_limit = operation == 2 ? 32 : 100;
        if (operation == 1) r->config.instruction_limit = 32;
        call_registration(c, source, candidate, method, route, FVM_ERROR);
        assert(list.object->resource_result.number == 80 && list.object->indexed.size() == before.size());
        for (unsigned long i = 0; i < 80; ++i)
            assert(list.object->indexed.find(i)->second.object == before.find(i)->second.object);
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->script_depth);
        r->config.instruction_limit = ordinary_limit;
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        v = call_registration(c, source, candidate, method, route, FVM_OK);
        assert(v.type == FVM_VALUE_BOOLEAN && v.boolean);
        assert(list.object->resource_result.number == (operation == 0 ? 81 : operation == 1 ? 79 : 80));
        /* Nested host operations must not reset the enclosing execution budget. */
        r->config.instruction_limit = 3;
        {
            FVM_ExecutionWorkScope outer(c);
            assert(outer.step() == FVM_OK && outer.step() == FVM_OK);
            call_registration(c, source, candidate, "addListener", 0, FVM_ERROR);
            assert(c->avm1_steps == 3 && c->avm1_execution_depth == 1);
        }
        assert(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth);
        r->config.instruction_limit = ordinary_limit;
        fvm_context_destroy(c);
    }
    /* Exact native boundary: two scans, two copies, one insertion. */
    FVM_Context *c = 0; assert(fvm_context_create(r, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    FVM_Object *source = 0, *a = 0, *b = 0, *candidate = 0;
    assert(fvm_object_create(c, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
    assert(fvm_object_create(c, &a) == FVM_OK && fvm_gc_pin(c, a) == FVM_OK);
    assert(fvm_object_create(c, &b) == FVM_OK && fvm_gc_pin(c, b) == FVM_OK);
    assert(fvm_object_create(c, &candidate) == FVM_OK && fvm_gc_pin(c, candidate) == FVM_OK);
    assert(fvm_broadcaster_initialize(c, source) == FVM_OK);
    call_registration(c, source, a, "addListener", 0, FVM_OK);
    call_registration(c, source, b, "addListener", 0, FVM_OK);
    r->config.instruction_limit = 4;
    call_registration(c, source, candidate, "addListener", 0, FVM_ERROR);
    assert(c->avm1_steps == 4);
    r->config.instruction_limit = 5;
    call_registration(c, source, candidate, "addListener", 0, FVM_OK);
    assert(c->avm1_steps == 5);
    /* Removing the first entry charges one lookup and all three stored entries. */
    r->config.instruction_limit = 3;
    call_registration(c, source, a, "removeListener", 0, FVM_ERROR);
    assert(c->avm1_steps == 3);
    r->config.instruction_limit = 4;
    call_registration(c, source, a, "removeListener", 0, FVM_OK);
    assert(c->avm1_steps == 4);
    /* Missing removal scans the list without changing it. Zero is unlimited. */
    r->config.instruction_limit = 1;
    call_registration(c, source, a, "removeListener", 0, FVM_ERROR);
    r->config.instruction_limit = 0;
    FVM_Value absent = call_registration(c, source, a, "removeListener", 0, FVM_OK);
    assert(absent.type == FVM_VALUE_BOOLEAN && !absent.boolean && c->avm1_steps == 2);
    assert(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth);
    r->config.instruction_limit = ordinary_limit;
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
}
