#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/extensions.h"
#include "../src/details/vector.hpp"
#include <cassert>
#include <cstring>

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
static void object(FVM_Value *v, FVM_Object *o)
{
    fvm_value_undefined(v);
    v->type = FVM_VALUE_OBJECT;
    v->object = o;
}
static void expect(FVM_Context *c, FVM_Object *o, const char *key, double expected)
{
    FVM_Value v;
    assert(fvm_object_get(c, o, key, &v) == FVM_OK && v.number == expected);
}
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *o = 0, *fn = 0, *a = 0;
    FVM_Channel *ch = 0;
    FVM_Value args[3], value, out, copy;
    int deleted;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    Bytes code;
    text(code, "previous");
    text(code, "old");
    code.push_back(0x1c);
    code.push_back(0x1d);
    text(code, "next");
    code.push_back(0x1c);
    text(code, "bias");
    code.push_back(0x1c);
    code.push_back(0x0a);
    code.push_back(0x3e);
    code.push_back(0);
    const char *params[] = {"key", "old", "next", "bias"};
    assert(fvm_function_create_avm1(c, code.begin(), code.size(), 4, params, 0, &fn) == FVM_OK);
    fvm_value_number(&value, 10);
    assert(fvm_object_set(c, o, "n", &value) == FVM_OK);
    fvm_value_string(args, "n");
    object(args + 1, fn);
    fvm_value_number(args + 2, 5);
    assert(fvm_object_call(c, o, "watch", 3, args, &out) == FVM_OK && out.boolean);
    fvm_value_number(&value, 20);
    assert(fvm_object_set(c, o, "n", &value) == FVM_OK);
    expect(c, o, "n", 25);
    assert(fvm_get_global(c, "previous", &out) == FVM_OK && out.number == 10);
    assert(fvm_object_delete(c, o, "n", &deleted) == FVM_OK && deleted);
    fvm_value_number(&value, 7);
    assert(fvm_object_set(c, o, "n", &value) == FVM_OK);
    expect(c, o, "n", 12);
    assert(fvm_get_global(c, "previous", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_construct(c, "Array", 0, 0, &a) == FVM_OK);
    fvm_value_string(args, "length");
    assert(fvm_object_call(c, a, "watch", 3, args, &out) == FVM_OK && !out.boolean);
    fvm_value_string(args, "0");
    assert(fvm_object_call(c, a, "watch", 3, args, &out) == FVM_OK && out.boolean);
    fvm_value_number(&value, 8);
    assert(fvm_table_set_index(c, a, 0, &value) == FVM_OK);
    expect(c, a, "0", 13);
    fvm_value_number(&value, 0);
    assert(fvm_object_set(c, a, "length", &value) == FVM_OK);
    fvm_value_number(&value, 8);
    assert(fvm_object_call(c, a, "push", 1, &value, &out) == FVM_OK);
    expect(c, a, "0", 13);
    fvm_value_number(&value, 4);
    assert(fvm_object_call(c, a, "unshift", 1, &value, &out) == FVM_OK);
    expect(c, a, "0", 9);
    assert(fvm_object_call(c, a, "reverse", 0, 0, &out) == FVM_OK);
    expect(c, a, "0", 18);
    /* A snapshot includes hidden watch descriptors and their Function roots. */
    fvm_value_string(args, "n");
    object(&value, o);
    assert(fvm_channel_create(&ch) == FVM_OK);
    assert(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK);
    assert(fvm_gc_pin(other, copy.object) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    fvm_value_number(&value, 30);
    assert(fvm_object_set(other, copy.object, "n", &value) == FVM_OK);
    expect(other, copy.object, "n", 35);
    assert(fvm_get_global(other, "previous", &out) == FVM_OK && out.number == 12);
    assert(fvm_object_call(other, copy.object, "unwatch", 1, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_call(other, copy.object, "unwatch", 1, args, &out) == FVM_OK && !out.boolean);
    fvm_value_number(&value, 30);
    assert(fvm_object_set(other, copy.object, "n", &value) == FVM_OK);
    expect(other, copy.object, "n", 30);
    /* Reentrant assignment to the same watchpoint does not invoke it again. */
    Bytes reentrant;
    text(reentrant, "this");
    reentrant.push_back(0x1c);
    text(reentrant, "n");
    text(reentrant, "next");
    reentrant.push_back(0x1c);
    reentrant.push_back(0x4f);
    text(reentrant, "old");
    reentrant.push_back(0x1c);
    reentrant.push_back(0x3e);
    reentrant.push_back(0);
    assert(fvm_function_create_avm1(c, reentrant.begin(), reentrant.size(), 4, params, 0, &fn) ==
           FVM_OK);
    object(args + 1, fn);
    assert(fvm_object_call(c, o, "watch", 3, args, &out) == FVM_OK && out.boolean);
    fvm_value_number(&value, 90);
    assert(fvm_object_set(c, o, "n", &value) == FVM_OK);
    expect(c, o, "n", 12);
    /* A thrown callback aborts assignment and propagates the original exception. */
    Bytes throwing;
    text(throwing, "watch rejected");
    throwing.push_back(0x2a);
    throwing.push_back(0);
    assert(fvm_function_create_avm1(c, throwing.begin(), throwing.size(), 0, 0, 0, &fn) == FVM_OK);
    object(args + 1, fn);
    assert(fvm_object_call(c, o, "watch", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_set(c, o, "n", &value) == FVM_EXCEPTION);
    expect(c, o, "n", 12);
    assert(fvm_exception_clear(c) == FVM_OK);
    /* Removing the watchpoint inside its callback must not drop its userdata
     * or receiver while the script explicitly runs the collector. */
    assert(fvm_extensions_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    FVM_Object *userdata = 0, *requested = 0;
    assert(fvm_construct(c, "Object", 0, 0, &userdata) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &requested) == FVM_OK);
    fvm_value_number(&value, 77);
    assert(fvm_object_set(c, userdata, "marker", &value) == FVM_OK);
    Bytes collecting;
    text(collecting, "n");
    const unsigned char one[] = {0x96, 5, 0, 7, 1, 0, 0, 0};
    const unsigned char zero[] = {0x96, 5, 0, 7, 0, 0, 0, 0};
    for (size_t i = 0; i < sizeof(one); ++i)
        collecting.push_back(one[i]);
    text(collecting, "this");
    collecting.push_back(0x1c);
    text(collecting, "unwatch");
    collecting.push_back(0x52);
    collecting.push_back(0x17);
    for (size_t i = 0; i < sizeof(zero); ++i)
        collecting.push_back(zero[i]);
    text(collecting, "gc");
    collecting.push_back(0x1c);
    text(collecting, "collect");
    collecting.push_back(0x52);
    collecting.push_back(0x17);
    text(collecting, "next");
    collecting.push_back(0x1c);
    collecting.push_back(0x3e);
    collecting.push_back(0);
    assert(fvm_function_create_avm1(c, collecting.begin(), collecting.size(), 4, params, 0, &fn) ==
           FVM_OK);
    object(args + 1, fn);
    object(args + 2, userdata);
    assert(fvm_object_call(c, o, "watch", 3, args, &out) == FVM_OK && out.boolean);
    object(&value, requested);
    assert(fvm_object_set(c, o, "n", &value) == FVM_OK);
    assert(fvm_object_get(c, o, "n", &out) == FVM_OK && out.object == requested);
    expect(c, userdata, "marker", 77);
    assert(fvm_object_call(c, o, "unwatch", 1, args, &out) == FVM_OK && !out.boolean);
    fvm_channel_destroy(ch);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
