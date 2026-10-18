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
static FVM_Object *argument;
static FVM_Object *later_argument;
static unsigned changes;
static FVM_Result change_argument(FVM_Context *c, FVM_Object *, unsigned int,
                                  const FVM_Value *, FVM_Value *out, void *)
{
    ++changes;
    FVM_Value v; fvm_value_number(&v, 70);
    assert(fvm_object_set(c, argument, "70000", &v) == FVM_OK);
    int deleted = 0;
    assert(fvm_object_delete(c, argument, "90000", &deleted) == FVM_OK && deleted);
    fvm_value_number(&v, 33);
    assert(fvm_table_set_index(c, later_argument, 3, &v) == FVM_OK);
    fvm_value_number(out, 100);
    return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    FVM_Object *array = 0, *proto = 0, *plain = 0;
    FVM_Value v, out, args[3];
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(&v, 1);
    assert(fvm_construct(c, "Array", 1, &v, &array) == FVM_OK);
    assert(fvm_table_set_index(c, array, 0, &v) == FVM_OK);
    fvm_value_number(&v, 100000);
    assert(fvm_construct(c, "Array", 1, &v, &argument) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
    assert(fvm_object_set_prototype(c, argument, proto) == FVM_OK);
    fvm_value_number(&v, 55);
    assert(fvm_object_set(c, argument, "90000", &v) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, proto, "0", change_argument, 0, 0, 0) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
    fvm_value_number(&v, 5);
    assert(fvm_object_set(c, plain, "length", &v) == FVM_OK);
    fvm_value_number(&v, 2);
    assert(fvm_construct(c, "Array", 1, &v, &later_argument) == FVM_OK);
    args[0] = object(argument); args[1] = object(later_argument); args[2] = object(plain);
    assert(fvm_object_call(c, array, "concat", 3, args, &out) == FVM_OK);
    assert(changes == 1);
    number(c, out.object, "length", 100006);
    number(c, out.object, "0", 1);
    number(c, out.object, "1", 100);
    number(c, out.object, "70001", 70);
    number(c, out.object, "100004", 33);
    int own = 1;
    assert(fvm_object_has_own(c, out.object, "90001", &own) == FVM_OK && !own);
    assert(fvm_table_get_index(c, out.object, 100005, &v) == FVM_OK && v.object == plain);
    /* Only the borrowed plain receiver spreads; plain arguments stay single values. */
    assert(borrow(c, array, "concat", plain, 1, args + 2, &out) == FVM_OK);
    number(c, out.object, "length", 6);
    assert(fvm_table_get_index(c, out.object, 5, &v) == FVM_OK && v.object == plain);
    rt->config.instruction_limit = 1;
    assert(fvm_object_call(c, array, "concat", 3, args, &out) == FVM_ERROR);
    assert(c->avm1_execution_depth == 0);
    rt->config.instruction_limit = 0;
    FVM_Object *overflow = 0;
    fvm_value_number(&v, 4294967295.0);
    assert(fvm_construct(c, "Array", 1, &v, &overflow) == FVM_OK);
    v = object(overflow);
    assert(fvm_object_call(c, array, "concat", 1, &v, &out) == FVM_INVALID_ARGUMENT);
    assert(c->avm1_execution_depth == 0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
