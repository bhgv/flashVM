#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>
#include <limits>

typedef fvm_detail::vector<unsigned char> Bytes;
static void text(Bytes &b, const char *s)
{
    size_t n = std::strlen(s) + 2;
    b.push_back(0x96);
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
    b.push_back(0);
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}
static void integer(Bytes &b, unsigned long n)
{
    b.push_back(0x96);
    b.push_back(5);
    b.push_back(0);
    b.push_back(7);
    for (unsigned int i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)n);
        n >>= 8;
    }
}
static void global(Bytes &b, const char *name)
{
    text(b, name);
    b.push_back(0x1c);
}
static FVM_Value run(FVM_Context *c, Bytes body)
{
    Bytes code;
    text(code, "result");
    for (size_t i = 0; i < body.size(); ++i)
        code.push_back(body[i]);
    code.push_back(0x1d);
    code.push_back(0);
    assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK);
    FVM_Value result;
    assert(fvm_get_global(c, "result", &result) == FVM_OK);
    return result;
}
static void object(FVM_Context *c, const char *name, FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    assert(fvm_set_global(c, name, &v) == FVM_OK);
}

#ifndef FVM_AVM1_STANDARD_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *o = 0, *clip = 0, *ctor = 0;
    FVM_Value v;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(&v, 7);
    assert(fvm_construct(c, "Number", 1, &v, &o) == FVM_OK);
    object(c, "boxed", o);
    assert(fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK);
    object(c, "clip", clip);
    const char *names[] = {"boxed", "clip", "Function", "String", "Math"};
    const char *types[] = {"object", "movieclip", "function", "function", "object"};
    for (unsigned int i = 0; i < 5; ++i)
    {
        Bytes b;
        global(b, names[i]);
        b.push_back(0x44);
        v = run(c, b);
        assert(v.type == FVM_VALUE_STRING && !std::strcmp(v.string, types[i]));
    }
    {
        Bytes b;
        global(b, "boxed");
        integer(b, 7);
        b.push_back(0x49);
        v = run(c, b);
        assert(v.type == FVM_VALUE_BOOLEAN && v.boolean);
        b[b.size() - 1] = 0x66;
        v = run(c, b);
        assert(!v.boolean);
    }
    {
        Bytes b;
        global(b, "clip");
        global(b, "clip");
        b.push_back(0x66);
        v = run(c, b);
        assert(v.boolean);
    }
    {
        Bytes b;
        text(b, "value=");
        global(b, "boxed");
        b.push_back(0x47);
        v = run(c, b);
        assert(!std::strcmp(v.string, "value=7"));
    }
    {
        /* U+10000 sorts before U+E000 in UTF-16, opposite UTF-8 byte order. */
        Bytes b;
        text(b, "\360\220\200\200");
        text(b, "\356\200\200");
        b.push_back(0x48);
        v = run(c, b);
        assert(v.boolean);
    }
    {
        Bytes b;
        integer(b, 1);
        integer(b, 1);
        text(b, "abc");
        text(b, "charAt");
        b.push_back(0x52);
        v = run(c, b);
        assert(!std::strcmp(v.string, "b"));
    }
    {
        Bytes b;
        text(b, "A\360\237\230\200Z");
        text(b, "length");
        b.push_back(0x4e);
        v = run(c, b);
        assert(v.type == FVM_VALUE_NUMBER && v.number == 4);
    }
    {
        Bytes b;
        text(b, "12.5");
        b.push_back(0x4a);
        v = run(c, b);
        assert(v.number == 12.5);
        Bytes text_code;
        integer(text_code, 15);
        text_code.push_back(0x4b);
        v = run(c, text_code);
        assert(!std::strcmp(v.string, "15"));
    }
    {
        /* NewMethod must execute a first-class constructor with the new receiver. */
        Bytes body;
        global(body, "this");
        text(body, "n");
        global(body, "n");
        body.push_back(0x4f);
        body.push_back(0);
        const char *parameters[] = {"n"};
        assert(fvm_function_create_avm1(c, body.begin(), body.size(), 1, parameters, 0, &ctor) ==
               FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = ctor;
        assert(fvm_object_set(c, o, "C", &v) == FVM_OK);
        object(c, "holder", o);
        Bytes b;
        integer(b, 19);
        integer(b, 1);
        global(b, "holder");
        text(b, "C");
        b.push_back(0x53);
        v = run(c, b);
        assert(v.type == FVM_VALUE_OBJECT);
        assert(fvm_object_get(c, v.object, "n", &v) == FVM_OK && v.number == 19);
    }
    {
        /* Negative numeric members are names, not unsigned table indices. */
        Bytes write;
        global(write, "holder");
        integer(write, 0xffffffffUL);
        integer(write, 55);
        write.push_back(0x4f);
        global(write, "holder");
        integer(write, 0xffffffffUL);
        write.push_back(0x4e);
        v = run(c, write);
        assert(v.type == FVM_VALUE_NUMBER && v.number == 55);
        object(c, "direct", ctor);
        Bytes call;
        integer(call, 23);
        integer(call, 1);
        global(call, "direct");
        call.push_back(0x96);
        call.push_back(1);
        call.push_back(0);
        call.push_back(3); /* Undefined method invokes the function itself. */
        call.push_back(0x52);
        v = run(c, call);
        assert(v.type == FVM_VALUE_UNDEFINED);
    }
    {
        fvm_value_number(&v, std::numeric_limits<double>::quiet_NaN());
        assert(fvm_set_global(c, "nan", &v) == FVM_OK);
        Bytes b;
        global(b, "nan");
        global(b, "nan");
        b.push_back(0x66);
        v = run(c, b);
        assert(!v.boolean);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}

#endif
