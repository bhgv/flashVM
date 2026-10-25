/* Focus transition arguments, committed errors, reentry and collection under notification. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cstring>


struct Trace
{
    FVM_Object *selection, *a, *b, *nested;
    unsigned calls;
    FVM_Object *receivers[12], *arguments[12];
    FVM_Result failure;
    bool reenter, collect;
    unsigned getters;
    FVM_Result getter_failure;
};


static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o;
    return v;
}


static FVM_Result select(FVM_Context *c, Trace *t, FVM_Object *o)
{
    FVM_Value arg = object(o), out;
    FVM_Result r = fvm_object_call(c, t->selection, "setFocus", 1, &arg, &out);
    if (r == FVM_OK)
    {
        if (!(out.type == FVM_VALUE_BOOLEAN && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return r;
}


static FVM_Result notify(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    Trace *t = (Trace *)raw;
    if (!(n == 1 && (args[0].type == FVM_VALUE_OBJECT || args[0].type == FVM_VALUE_NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned i = t->calls++;
    if (!(i < 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->receivers[i] = self;
    t->arguments[i] = args[0].type == FVM_VALUE_OBJECT ? args[0].object : 0;
    if (t->reenter)
    {
        t->reenter = false;
        if (!(select(c, t, t->nested) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (t->collect)
    { /* Trusted test entry verifies the transition's own roots, without native barriers. */
        if (!(!c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value alive;
        if (!(fvm_object_get(c, self, "focusEnabled", &alive) == FVM_OK && alive.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (args[0].type == FVM_VALUE_OBJECT)
        {
            if (!(fvm_object_get(c, args[0].object, "focusEnabled", &alive) == FVM_OK && alive.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_value_undefined(out);
    return t->failure;
}


static FVM_Result getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Trace *t = (Trace *)raw;
    ++t->getters;
    fvm_value_number(out, 12); /* Noncallable getter value is silently ignored. */
    return t->getter_failure;
}


static FVM_Object *clip(FVM_Context *c, const char *name, Trace *t)
{
    FVM_Object *o = 0;
    FVM_Value yes;
    fvm_value_boolean(&yes, 1);
    if (!(fvm_movieclip_create(c, name, 1, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, o, "focusEnabled", &yes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, o, "onKillFocus", notify, t, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, o, "onSetFocus", notify, t, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o->methods["onKillFocus"].collector_entry = true;
    o->methods["onSetFocus"].collector_entry = true;
    return o;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_selection_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Trace t;
    std::memset(&t, 0, sizeof(t));
    t.failure = FVM_OK;
    FVM_Value ns;
    if (!(fvm_get_global(c, "Selection", &ns) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.selection = ns.object;
    t.a = clip(c, "a", &t);
    t.b = clip(c, "b", &t);
    t.nested = clip(c, "nested", &t);
    if (!(fvm_gc_pin(c, t.a) == FVM_OK && fvm_gc_pin(c, t.b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, t.nested) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(select(c, &t, t.a) == FVM_OK && t.calls == 1 && !t.arguments[0]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(select(c, &t, t.a) == FVM_OK && t.calls == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(select(c, &t, t.b) == FVM_OK && t.calls == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.receivers[1] == t.a && t.arguments[1] == t.b))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.receivers[2] == t.b && t.arguments[2] == t.a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(select(c, &t, 0) == FVM_OK && t.calls == 4 && !t.arguments[3]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(select(c, &t, 0) == FVM_OK && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Native host operations remain silent. Invalid handler status must not become false. */
    if (!(fvm_input_set_keyboard_focus(c, t.a) == FVM_OK && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.failure = FVM_INVALID_ARGUMENT;
    if (!(select(c, &t, t.b) == FVM_INVALID_ARGUMENT && c->keyboard_focus == t.b && t.calls == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.failure = FVM_UNSUPPORTED;
    if (!(select(c, &t, 0) == FVM_UNSUPPORTED && !c->keyboard_focus && t.calls == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.failure = FVM_OK;
    t.calls = 0;
    if (!(fvm_input_set_keyboard_focus(c, t.a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.reenter = true;
    if (!(select(c, &t, t.b) == FVM_OK && c->keyboard_focus == t.nested && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.receivers[0] == t.a && t.arguments[0] == t.b))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.receivers[1] == t.b && t.arguments[1] == t.nested))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.receivers[2] == t.nested && t.arguments[2] == t.b))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.receivers[3] == t.b && t.arguments[3] == t.a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Inherited handlers keep the child receiver; handler getters run exactly once. */
    FVM_Object *proto = 0;
    if (!(fvm_object_create(c, &proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, proto, "onSetFocus", notify, &t, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.b->methods.erase("onSetFocus");
    t.b->prototype = proto;
    if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.calls = 0;
    if (!(select(c, &t, t.b) == FVM_OK && t.calls == 1 && t.receivers[0] == t.b))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, t.b, "onSetFocus", getter, 0, &t, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.calls = 0;
    t.getter_failure = FVM_OK;
    if (!(select(c, &t, t.b) == FVM_OK && !t.calls && t.getters == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.getter_failure = FVM_INVALID_ARGUMENT;
    if (!(select(c, &t, t.b) == FVM_INVALID_ARGUMENT && c->keyboard_focus == t.b && t.getters == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.b->accessors.erase("onSetFocus");
    /* Missing optional methods must not invoke a global callback with the same name. */
    proto->methods.erase("onSetFocus");
    if (!(fvm_register_function(c, "onSetFocus", notify, &t) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.calls = 0;
    if (!(select(c, &t, t.b) == FVM_OK && !t.calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, t.b, "onSetFocus", notify, &t, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.b->methods["onSetFocus"].collector_entry = true;
    /* Both endpoints lose all persistent roots inside the first callback and remain alive. */
    if (!(fvm_input_set_keyboard_focus(c, t.a) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, t.a) == FVM_OK && fvm_gc_unpin(c, t.b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.calls = 0;
    t.collect = true;
    if (!(select(c, &t, t.b) == FVM_OK && t.calls == 2 && !c->keyboard_focus))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, t.nested) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
