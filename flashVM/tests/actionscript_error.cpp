#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
typedef fvm_detail::vector<unsigned char> Bytes;
static void word(Bytes &b, size_t n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}
static void text(Bytes &b, const char *s)
{
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}
static void string(Bytes &b, const char *s)
{
    b.push_back(0x96);
    word(b, std::strlen(s) + 2);
    b.push_back(0);
    text(b, s);
}
static void integer(Bytes &b, unsigned long n)
{
    b.push_back(0x96);
    word(b, 5);
    b.push_back(7);
    for (int i = 0; i < 4; ++i)
        b.push_back((unsigned char)(n >> (i * 8)));
}
static void variable(Bytes &b, const char *s)
{
    string(b, s);
    b.push_back(0x1c);
}
static void append(Bytes &b, const Bytes &src)
{
    for (size_t i = 0; i < src.size(); ++i)
        b.push_back(src[i]);
}
static void method(Bytes &b, const char *receiver, const char *name)
{
    integer(b, 0);
    variable(b, receiver);
    string(b, name);
    b.push_back(0x52);
}
static void run(FVM_Context *c, Bytes b)
{
    b.push_back(0);
    assert(fvm_avm1_execute(c, &b[0], b.size()) == FVM_OK);
}
static FVM_Result raises(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *,
                         void *)
{
    FVM_Value error;
    fvm_value_string(&error, "message getter failed");
    return fvm_exception_raise(c, &error);
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Value value, klass, prototype;
    Bytes create, body, handler, final, actions;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    string(create, "failure");
    string(create, "script failure");
    integer(create, 1);
    string(create, "Error");
    create.push_back(0x40);
    create.push_back(0x1d);
    run(c, create);
    variable(body, "failure");
    body.push_back(0x2a);
    string(handler, "observed");
    integer(handler, 0);
    variable(handler, "caught");
    string(handler, "toString");
    handler.push_back(0x52);
    handler.push_back(0x1d);
    string(final, "finished");
    integer(final, 1);
    final.push_back(0x1d);
    actions.push_back(0x8f);
    word(actions, 7 + 7);
    actions.push_back(3);
    word(actions, body.size());
    word(actions, handler.size());
    word(actions, final.size());
    text(actions, "caught");
    append(actions, body);
    append(actions, handler);
    append(actions, final);
    run(c, actions);
    assert(fvm_get_global(c, "observed", &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
           !std::strcmp(value.string, "script failure"));
    assert(fvm_get_global(c, "finished", &value) == FVM_OK && value.number == 1);
    assert(fvm_exception_get(c, &value) == FVM_NOT_FOUND);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    Bytes after_gc;
    string(after_gc, "afterGC");
    method(after_gc, "failure", "toString");
    after_gc.push_back(0x1d);
    run(c, after_gc);
    assert(fvm_get_global(c, "afterGC", &value) == FVM_OK &&
           !std::strcmp(value.string, "script failure"));
    Bytes type_check;
    string(type_check, "isError");
    variable(type_check, "failure");
    variable(type_check, "Error");
    type_check.push_back(0x54);
    type_check.push_back(0x1d);
    run(c, type_check);
    assert(fvm_get_global(c, "isError", &value) == FVM_OK && value.type == FVM_VALUE_BOOLEAN &&
           value.boolean);
    /* A getter throw from Error.toString propagates through AVM1 to catch. */
    assert(fvm_get_global(c, "Error", &klass) == FVM_OK);
    assert(fvm_object_get(c, klass.object, "prototype", &prototype) == FVM_OK);
    FVM_Object *custom = 0;
    assert(fvm_object_create(c, &custom) == FVM_OK);
    assert(fvm_object_set_prototype(c, custom, prototype.object) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, custom, "message", raises, 0, 0, 0) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = custom;
    assert(fvm_set_global(c, "getterFailure", &value) == FVM_OK);
    Bytes getter, catch_getter, try_getter;
    method(getter, "getterFailure", "toString");
    string(catch_getter, "getterObserved");
    variable(catch_getter, "caught");
    catch_getter.push_back(0x1d);
    try_getter.push_back(0x8f);
    word(try_getter, 14);
    try_getter.push_back(1);
    word(try_getter, getter.size());
    word(try_getter, catch_getter.size());
    word(try_getter, 0);
    text(try_getter, "caught");
    append(try_getter, getter);
    append(try_getter, catch_getter);
    run(c, try_getter);
    assert(fvm_get_global(c, "getterObserved", &value) == FVM_OK &&
           !std::strcmp(value.string, "message getter failed"));
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
