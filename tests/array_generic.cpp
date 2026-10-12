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
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *array = 0, *o = 0, *proto = 0, *inherited = 0, *base = 0, *handler = 0;
    FVM_Channel *channel = 0;
    FVM_Value out, args[3], method, copy;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
    assert(fvm_object_set_prototype(c, o, proto) == FVM_OK);
    fvm_value_number(args, 3);
    assert(fvm_object_set(c, o, "length", args) == FVM_OK);
    fvm_value_string(args, "A");
    assert(fvm_object_set(c, o, "0", args) == FVM_OK);
    fvm_value_string(args, "B");
    assert(fvm_object_set(c, proto, "1", args) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, proto, "2", getter, 0, 0, 0) == FVM_OK);
    assert(borrow(c, array, "join", o, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, "A,B,4"));
    assert(reads == 1);
    assert(borrow(c, array, "shift", o, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, "A"));
    number(c, o, "length", 2);
    number(c, o, "1", 4);
    assert(fvm_object_get(c, o, "0", &out) == FVM_OK && !std::strcmp(out.string, "B"));
    /* Restore a plain prototype before extending beyond an inherited getter. */
    assert(fvm_object_set_prototype(c, o, 0) == FVM_OK);
    fvm_value_number(args, 7);
    assert(borrow(c, array, "push", o, 1, args, &out) == FVM_OK && out.number == 3);
    assert(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.number == 7);
    fvm_value_number(args, 8);
    assert(borrow(c, array, "unshift", o, 1, args, &out) == FVM_OK && out.number == 3);
    number(c, o, "0", 8);
    number(c, o, "2", 4);
    assert(borrow(c, array, "reverse", o, 0, 0, &out) == FVM_OK && out.object == o);
    number(c, o, "0", 4);
    number(c, o, "2", 8);
    fvm_value_number(args, 1);
    fvm_value_number(args + 1, 3);
    assert(borrow(c, array, "slice", o, 2, args, &out) == FVM_OK);
    number(c, out.object, "length", 2);
    number(c, out.object, "1", 8);
    fvm_value_number(args, 1);
    fvm_value_number(args + 1, 1);
    fvm_value_number(args + 2, 9);
    assert(borrow(c, array, "splice", o, 3, args, &out) == FVM_OK);
    number(c, out.object, "length", 1);
    number(c, o, "1", 9);
    fvm_value_number(args, 16);
    assert(borrow(c, array, "sort", o, 1, args, &out) == FVM_OK && out.object == o);
    number(c, o, "0", 4);
    number(c, o, "1", 8);
    number(c, o, "2", 9);
    fvm_value_number(args, 2);
    assert(borrow(c, array, "concat", o, 1, args, &out) == FVM_OK);
    number(c, out.object, "length", 2);
    assert(fvm_object_get(c, out.object, "0", &copy) == FVM_OK && copy.object == o);
    /* A real Array reads inherited slots and creates destination own properties. */
    fvm_value_number(args, 4);
    assert(fvm_construct(c, "Array", 1, args, &inherited) == FVM_OK);
    assert(fvm_object_get_prototype(c, inherited, &base) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK);
    assert(fvm_object_set_prototype(c, proto, base) == FVM_OK);
    fvm_value_number(args, 10);
    assert(fvm_object_set(c, proto, "1", args) == FVM_OK);
    assert(fvm_object_set_prototype(c, inherited, proto) == FVM_OK);
    fvm_value_number(args, 1);
    assert(fvm_object_set(c, inherited, "0", args) == FVM_OK);
    fvm_value_number(args, 4);
    assert(fvm_object_set(c, inherited, "3", args) == FVM_OK);
    assert(fvm_object_call(c, inherited, "reverse", 0, 0, &out) == FVM_OK &&
           out.object == inherited);
    assert(fvm_object_call(c, inherited, "join", 0, 0, &out) == FVM_OK &&
           !std::strcmp(out.string, "4,10,10,1"));
    assert(fvm_object_call(c, inherited, "slice", 0, 0, &out) == FVM_OK);
    number(c, out.object, "1", 10);
    number(c, out.object, "2", 10);
    /* Borrowed join also terminates a recursive generic object's toString. */
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    fvm_value_number(args, 1);
    assert(fvm_object_set(c, o, "length", args) == FVM_OK);
    out = object(o);
    assert(fvm_object_set(c, o, "0", &out) == FVM_OK);
    assert(fvm_object_get(c, array, "join", &method) == FVM_OK);
    assert(fvm_object_set(c, o, "toString", &method) == FVM_OK);
    assert(borrow(c, array, "join", o, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, ""));
    /* Watch writes run on the actual receiver, not on a sorting snapshot. */
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    fvm_value_number(args, 0);
    assert(fvm_object_set(c, o, "length", args) == FVM_OK);
    const unsigned char code[] = {0x96, 5, 0, 7, 9, 0, 0, 0, 0x3e, 0};
    assert(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &handler) == FVM_OK);
    fvm_value_string(args, "0");
    args[1] = object(handler);
    assert(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean);
    fvm_value_number(args, 11);
    assert(borrow(c, array, "push", o, 1, args, &out) == FVM_OK);
    number(c, o, "0", 9);
    /* Numeric accessor deletion removes the descriptor, not only an index slot. */
    assert(fvm_object_define_accessor_rooted(c, o, "3", getter, 0, 0, 0) == FVM_OK);
    int deleted = 0, own = 0;
    assert(fvm_object_has_own(c, o, "3", &own) == FVM_OK && own);
    assert(fvm_object_delete(c, o, "3", &deleted) == FVM_OK && deleted);
    assert(fvm_object_has_own(c, o, "3", &own) == FVM_OK && !own);
    /* Imported method captures allow the same borrow in a fresh context. */
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_object_get(c, array, "push", &method) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &method, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_object_create(other, &o) == FVM_OK);
    fvm_value_number(args, 0);
    assert(fvm_object_set(other, o, "length", args) == FVM_OK);
    args[0] = object(o);
    fvm_value_number(args + 1, 23);
    assert(fvm_object_call(other, copy.object, "call", 2, args, &out) == FVM_OK && out.number == 1);
    assert(fvm_object_get(other, o, "0", &out) == FVM_OK && out.number == 23);
    number(other, o, "length", 1);
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
