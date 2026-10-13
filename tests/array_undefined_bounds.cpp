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
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    FVM_Object *array = 0, *plain = 0;
    FVM_Value v, out, args[3];
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(&v, 3);
    assert(fvm_construct(c, "Array", 1, &v, &array) == FVM_OK);
    fvm_value_number(&v, 9);
    assert(fvm_table_set_index(c, array, 2, &v) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, plain, "length", getter, 0, 0, 0) == FVM_OK);
    for (unsigned i = 0; i < 2; ++i) {
        FVM_Object *receiver = i ? plain : array;
        fvm_value_undefined(args);
        assert(borrow(c, array, "splice", receiver, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        assert(borrow(c, array, "splice", receiver, 1, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    }
    assert(reads == 0);
    for (unsigned i = 0; i < 2; ++i) {
        FVM_Object *receiver = i ? plain : array;
        fvm_value_number(args, 1);
        fvm_value_undefined(args + 1);
        fvm_value_number(args + 2, 99);
        assert(borrow(c, array, "splice", receiver, 3, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        fvm_value_number(args + 1, -1);
        assert(borrow(c, array, "splice", receiver, 3, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    }
    number(c, array, "length", 3);
    number(c, array, "2", 9);
    int own = 1;
    assert(fvm_object_has_own(c, plain, "1", &own) == FVM_OK && !own);
    fvm_value_number(args, 1);
    fvm_value_undefined(args + 1);
    assert(borrow(c, array, "slice", array, 2, args, &out) == FVM_OK);
    number(c, out.object, "length", 2);
    number(c, out.object, "1", 9);
    /* Zero deletion is valid and inserts values, unlike a negative count. */
    fvm_value_number(args + 1, 0);
    assert(borrow(c, array, "splice", array, 3, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    number(c, out.object, "length", 0);
    number(c, array, "length", 4);
    number(c, array, "1", 99);
    fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_NULL;
    fvm_value_number(args + 2, 88);
    assert(borrow(c, array, "splice", array, 3, args, &out) == FVM_OK);
    number(c, array, "length", 5);
    number(c, array, "1", 88);
    /* Omitted deletion count still removes the remaining tail. */
    assert(borrow(c, array, "splice", array, 1, args, &out) == FVM_OK);
    number(c, out.object, "length", 4);
    number(c, out.object, "3", 9);
    number(c, array, "length", 1);
    /* An explicit undefined count is rejected before the generic size cap. */
    FVM_Object *huge = 0;
    assert(fvm_construct(c, "Object", 0, 0, &huge) == FVM_OK);
    fvm_value_number(&v, 4294967295.0);
    assert(fvm_object_set(c, huge, "length", &v) == FVM_OK);
    fvm_value_undefined(args + 1);
    assert(borrow(c, array, "splice", huge, 2, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    number(c, huge, "length", 4294967295.0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
