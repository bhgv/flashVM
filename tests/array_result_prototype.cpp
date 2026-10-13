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
#include "flashvm/gc.h"
#include "flashvm/userdata.h"
#include "flashvm/avm1.h"
static FVM_Object *replacement;
static unsigned prototype_reads, released;
static void release_prototype(void *) { ++released; }
static FVM_Result replacement_getter(FVM_Context *, FVM_Object *, unsigned int,
                                     const FVM_Value *, FVM_Value *out, void *)
{
    ++prototype_reads;
    *out = object(replacement);
    return FVM_OK;
}
static void has_prototype(FVM_Context *c, FVM_Object *array, FVM_Object *expected)
{
    FVM_Object *p = 0;
    assert(fvm_object_get_prototype(c, array, &p) == FVM_OK && p == expected);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    FVM_Object *original = 0, *source = 0, *custom = 0, *made = 0;
    FVM_Value klass, v, out, args[2];
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "Array", &klass) == FVM_OK);
    assert(fvm_object_get(c, klass.object, "prototype", &v) == FVM_OK);
    original = v.object;
    assert(fvm_userdata_attach(c, original, 0, release_prototype) == FVM_OK);
    fvm_value_number(&v, 3);
    assert(fvm_construct(c, "Array", 1, &v, &source) == FVM_OK);
    assert(fvm_gc_pin(c, source) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &custom) == FVM_OK);
    assert(fvm_object_set_prototype(c, source, custom) == FVM_OK);
    fvm_value_number(&v, 77);
    assert(fvm_object_set(c, custom, "source_marker", &v) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, custom, "1", getter, 0, 0, 0) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &replacement) == FVM_OK);
    assert(fvm_object_set_property_flags(c, klass.object, "prototype", 0, 7) == FVM_OK);
    int deleted = 0;
    assert(fvm_object_delete(c, klass.object, "prototype", &deleted) == FVM_OK && deleted);
    assert(fvm_object_define_accessor_rooted(c, klass.object, "prototype", replacement_getter,
                                            0, 0, replacement) == FVM_OK);
    /* No array or fetched method retains the original prototype at this collection. */
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 0);
    fvm_value_number(args, 1); fvm_value_number(args + 1, 2);
    assert(borrow(c, original, "slice", source, 2, args, &out) == FVM_OK);
    has_prototype(c, out.object, original);
    number(c, out.object, "length", 1); number(c, out.object, "0", 4);
    assert(fvm_object_get(c, out.object, "source_marker", &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_get(c, out.object, "1", &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED);
    assert(borrow(c, original, "concat", source, 0, 0, &out) == FVM_OK);
    has_prototype(c, out.object, original);
    fvm_value_number(args, 1); fvm_value_number(args + 1, 1);
    assert(borrow(c, original, "splice", source, 2, args, &out) == FVM_OK);
    has_prototype(c, out.object, original);
    number(c, out.object, "0", 4);
    fvm_value_number(args, 8);
    assert(borrow(c, original, "sort", source, 1, args, &out) == FVM_OK);
    has_prototype(c, out.object, original);
    FVM_Object *text = 0;
    fvm_value_string(&v, "a:b");
    assert(fvm_construct(c, "String", 1, &v, &text) == FVM_OK);
    fvm_value_string(args, ":");
    assert(fvm_object_call(c, text, "split", 1, args, &out) == FVM_OK);
    has_prototype(c, out.object, original);
    const unsigned char actions[] = {
        0x96, 9, 0, 0, 'm', 'a', 'd', 'e', 'A', 'V', 'M', 0,
        0x96, 5, 0, 7, 0, 0, 0, 0, 0x42, 0x1d, 0
    };
    assert(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK);
    assert(fvm_get_global(c, "madeAVM", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    has_prototype(c, out.object, original);
    assert(prototype_reads == 0);
    /* Explicit construction still honors the public prototype, reading its getter once. */
    assert(fvm_construct(c, "Array", 0, 0, &made) == FVM_OK && prototype_reads == 1);
    has_prototype(c, made, replacement);
    assert(fvm_gc_unpin(c, source) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
    assert(released == 1);
}
