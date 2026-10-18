/* Ordered native target projection and script argument conversions for ASSetPropFlags. */
#ifndef FVM_TEST_PROPERTY_FLAGS_FIXTURE_HPP
#define FVM_TEST_PROPERTY_FLAGS_FIXTURE_HPP
#include "primitive_boxing_fixture.hpp"
struct FlagFixture;
struct FlagEvent { FlagFixture *fixture; char role; };
struct FlagFixture
{
    FVM_Context *context;
    FVM_Object *target, *factory;
    FVM_Value input, args[4];
    FlagEvent events[3];
    char trace[32], fault;
    unsigned used;
    bool exception, mutate;
};
static FVM_Result flag_event(FlagFixture &f, char role, FVM_Value *out)
{
    assert(f.used < sizeof(f.trace)); f.trace[f.used++] = role;
    assert(fvm_gc_collect(f.context, 0) == FVM_INVALID_ARGUMENT);
    if (f.fault != role) return FVM_OK;
    if (!f.exception) return FVM_ERROR;
    *out = class_value(f.target); return fvm_exception_raise(f.context, out);
}
static FVM_Result flag_factory(FVM_Context *, FVM_Object *, unsigned count,
                                const FVM_Value *args, FVM_Value *out, void *raw)
{
    FlagFixture &f = *(FlagFixture *)raw;
    assert(count == 1); boxing_equal(args[0], f.input);
    FVM_Result status = flag_event(f, 'T', out);
    if (status == FVM_OK) *out = class_value(f.target);
    return status;
}
static FVM_Result flag_argument(FVM_Context *c, const FVM_ScriptFrame *frame,
                                 FVM_Value *out, void *raw)
{
    FlagEvent *event = (FlagEvent *)raw; FlagFixture &f = *event->fixture;
    assert(frame->capture == f.target);
    FVM_Result status = flag_event(f, event->role, out);
    if (status != FVM_OK) return status;
    if (event->role == 'S') { fvm_value_number(out, 3.9); return FVM_OK; }
    if (event->role == 'C') { fvm_value_number(out, 1.9); return FVM_OK; }
    if (f.mutate) {
        status = fvm_object_set_property_flags_bytes(c, f.target, "p\0q", 3, 4, 0);
        if (status != FVM_OK) return status;
    }
    return fvm_value_string_bytes(out, "p\0q,z", 5);
}
static void flag_init(FVM_Runtime *rt, FlagFixture &f, unsigned version, unsigned type)
{
    f.context = 0; f.used = 0; f.fault = 0; f.exception = false; f.mutate = true;
    assert(fvm_context_create(rt, &f.context) == FVM_OK);
    FVM_Context *c = f.context;
    unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK && fvm_construct(c, "Object", 0, 0, &f.target) == FVM_OK);
    FVM_Value value; fvm_value_number(&value, 1);
    assert(fvm_object_set_bytes(c, f.target, "p\0q", 3, &value) == FVM_OK && fvm_object_set(c, f.target, "z", &value) == FVM_OK);
    value = class_value(f.target);
    assert(fvm_object_set(c, f.target, "self", &value) == FVM_OK && fvm_set_global(c, "flagTarget", &value) == FVM_OK);
    assert(fvm_function_create_native(c, flag_factory, &f, f.target, &f.factory) == FVM_OK);
    f.factory->constructable = f.factory->invocable;
    const char *names[] = {"Number", "Boolean", "String"};
    value = class_value(f.factory); assert(fvm_set_global(c, names[type], &value) == FVM_OK);
    if (!type) fvm_value_number(&f.input, 42);
    if (type == 1) fvm_value_boolean(&f.input, 0);
    if (type == 2) assert(fvm_value_string_bytes(&f.input, "a\0b", 3) == FVM_OK);
    f.args[0] = f.input;
    const char roles[] = {'N', 'S', 'C'};
    const char *globals[] = {"flagNames", "flagSet", "flagClear"};
    for (unsigned i = 0; i < 3; ++i) {
        f.events[i].fixture = &f; f.events[i].role = roles[i];
        FVM_Object *object = 0, *function = 0; const unsigned char body[] = {0};
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &function) == FVM_OK && fvm_function_set_body(c, function, flag_argument, f.events + i, f.target) == FVM_OK);
        value = class_value(function); assert(fvm_object_set(c, object, i ? "valueOf" : "toString", &value) == FVM_OK);
        f.args[i + 1] = class_value(object); assert(fvm_set_global(c, globals[i], f.args + i + 1) == FVM_OK);
    }
    assert(fvm_set_global(c, "flagInput", &f.input) == FVM_OK);
}
static void flag_check(FVM_Context *c, FVM_Object *target, unsigned expected)
{
    unsigned actual = 99;
    assert(fvm_object_property_flags_bytes(c, target, "p\0q", 3, &actual) == FVM_OK && actual == expected);
    assert(fvm_object_property_flags(c, target, "z", &actual) == FVM_OK && actual == (expected & 3U));
}
#endif
