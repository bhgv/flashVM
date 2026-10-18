/* Native boxing can return an actual watcher Function rather than an ordinary wrapper. */
#ifndef FVM_TEST_WATCH_BOXING_FIXTURE_HPP
#define FVM_TEST_WATCH_BOXING_FIXTURE_HPP
#include "primitive_boxing_fixture.hpp"
struct WatchBoxingFixture
{
    FVM_Context *context;
    FVM_Object *target, *handler, *factory;
    FVM_Value input;
    unsigned mode, calls, version;
};
static FVM_Result watch_box_factory(FVM_Context *c, FVM_Object *, unsigned n,
                                     const FVM_Value *args, FVM_Value *out, void *raw)
{
    WatchBoxingFixture *f = (WatchBoxingFixture *)raw;
    ++f->calls; assert(n == 1); boxing_equal(args[0], f->input);
    *out = class_value(f->mode == 1 ? f->target : f->handler);
    if (f->mode == 2) fvm_value_number(out, 99);
    if (f->mode == 3) return FVM_ERROR;
    if (f->mode == 4) { *out = class_value(f->target); return fvm_exception_raise(c, out); }
    return FVM_OK;
}
static void watch_box_init(FVM_Runtime *rt, WatchBoxingFixture &f, unsigned version,
                             unsigned type, unsigned mode)
{
    f.context = 0; f.mode = mode; f.calls = 0; f.version = version;
    assert(fvm_context_create(rt, &f.context) == FVM_OK);
    FVM_Context *c = f.context;
    if (version) {
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
    }
    assert(fvm_standard_install(c) == FVM_OK && fvm_construct(c, "Object", 0, 0, &f.target) == FVM_OK);
    FVM_Value value = class_value(f.target);
    assert(fvm_set_global(c, "watchTarget", &value) == FVM_OK && fvm_object_set(c, f.target, "self", &value) == FVM_OK);
    Program body; body.variable("this"); body.text("seen"); body.variable("userData"); body.op(0x4f);
    body.variable("newValue"); body.variable("userData"); body.op(0x0a); body.finish();
    const char *parameters[] = {"name", "oldValue", "newValue", "userData"};
    assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 4, parameters, 0, &f.handler) == FVM_OK);
    assert(fvm_function_create_native(c, watch_box_factory, &f, f.handler, &f.factory) == FVM_OK);
    f.factory->constructable = f.factory->invocable;
    const char *names[] = {"Number", "Boolean", "String"};
    value = class_value(f.factory); assert(fvm_set_global(c, names[type], &value) == FVM_OK);
    if (!type) fvm_value_number(&f.input, 42);
    if (type == 1) fvm_value_boolean(&f.input, 0);
    if (type == 2) assert(fvm_value_string_bytes(&f.input, "a\0b", 3) == FVM_OK);
    assert(fvm_set_global(c, "watchInput", &f.input) == FVM_OK);
}
#endif
