#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <cassert>
#include <cstring>
static unsigned int reads;
static FVM_Result getter(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    ++reads;
    fvm_value_number(out, 4);
    return FVM_OK;
}
static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}
static FVM_Result borrow(FVM_Context *c, FVM_Object *array, const char *name, FVM_Object *receiver,
                         unsigned int n, const FVM_Value *args, FVM_Value *out)
{
    FVM_Value method, call[8];
    assert(n < 7);
    assert(fvm_object_get(c, array, name, &method) == FVM_OK && method.type == FVM_VALUE_OBJECT);
    call[0] = object(receiver);
    for (unsigned int i = 0; i < n; ++i)
        call[i + 1] = args[i];
    return fvm_object_call(c, method.object, "call", n + 1, call, out);
}
static void number(FVM_Context *c, FVM_Object *o, const char *name, double expected)
{
    FVM_Value v;
    assert(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER &&
           v.number == expected);
}
#include "../src/core/runtime.hpp"
#include <cstdio>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; FVM_Object *array = 0, *o = 0, *proto = 0;
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        swf[3] = (unsigned char)version;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
        FVM_Value v, out;
        fvm_value_string(&v, "inherited");
        assert(fvm_object_set(c, proto, "3", &v) == FVM_OK);
        for (unsigned real_array = 0; real_array < 2; ++real_array) {
            fvm_value_number(&v, 4);
            assert(fvm_construct(c, real_array ? "Array" : "Object", real_array ? 1 : 0,
                                 real_array ? &v : 0, &o) == FVM_OK);
            assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
            assert(fvm_object_set(c, o, "length", &v) == FVM_OK);
            fvm_value_string(&v, "A"); assert(fvm_object_set(c, o, "0", &v) == FVM_OK);
            fvm_value_undefined(&v); v.type = FVM_VALUE_NULL;
            assert(fvm_object_set(c, o, "2", &v) == FVM_OK);
            assert(fvm_object_define_accessor_rooted(c, o, "1", getter, 0, 0, 0) == FVM_OK);
            reads = 0;
            assert(borrow(c, array, "join", o, 0, 0, &out) == FVM_OK && reads == 0);
            assert(!std::strcmp(out.string, version < 7 ? "A,,null," : "A,undefined,null,undefined"));
            fvm_value_undefined(&v);
            assert(borrow(c, array, "join", o, 1, &v, &out) == FVM_OK && reads == 0);
            assert(!std::strcmp(out.string, version < 7 ? "Anull" :
                               "A" "undefined" "undefined" "undefined" "null" "undefined" "undefined"));
        }
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        assert(fvm_object_define_accessor_rooted(c, o, "length", getter, 0, 0, 0) == FVM_OK);
        reads = 0;
        assert(borrow(c, array, "join", o, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, "") && reads == 0);
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        fvm_value_number(&v, 4294967295.0); assert(fvm_object_set(c, o, "length", &v) == FVM_OK);
        assert(borrow(c, array, "join", o, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, ""));
        fvm_value_number(&v, 2147483647.0);
        assert(fvm_object_set(c, o, "length", &v) == FVM_OK);
        rt->config.instruction_limit = 64;
        assert(borrow(c, array, "join", o, 0, 0, &out) == FVM_ERROR);
        assert(c->avm1_execution_depth == 0 && c->joining_arrays.empty());
        rt->config.instruction_limit = 0;
        FVM_Object *property_object = 0, *get = 0;
        assert(fvm_construct(c, "Object", 0, 0, &property_object) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get) == FVM_OK);
        FVM_Value descriptor[3];
        fvm_value_undefined(descriptor); descriptor[1] = object(get);
        fvm_value_undefined(descriptor + 2); descriptor[2].type = FVM_VALUE_NULL;
        assert(fvm_object_call(c, property_object, "addProperty", 3, descriptor, &out) == FVM_OK);
        assert(out.type == FVM_VALUE_BOOLEAN && out.boolean == (version >= 7));
        if (version >= 7) number(c, property_object, "undefined", 4);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
