#include "flashvm/value.h"
#include "flashvm/gc.h"
#include "flashvm/userdata.h"
#include "flashvm/avm1.h"
#include <assert.h>
#include <string.h>
static FVM_Object *receiver;
static double stored;
static unsigned int released;
static void release(void *p)
{
    assert(p == &released);
    ++released;
}
static FVM_Result getter(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                         FVM_Value *out, void *user)
{
    (void)a;
    assert(self == receiver && n == 0 && user);
    assert(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
    fvm_value_number(out, stored);
    return FVM_OK;
}
static FVM_Result setter(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                         FVM_Value *out, void *user)
{
    (void)c;
    assert(self == receiver && n == 1 && user);
    if (a[0].type != FVM_VALUE_NUMBER)
        return FVM_INVALID_ARGUMENT;
    stored = a[0].number;
    fvm_value_undefined(out);
    return FVM_OK;
}
static FVM_Result constructor(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                              FVM_Value *out, void *user)
{
    FVM_Object *object = 0;
    (void)self;
    assert(user && n == 2 && a[0].number == 10 && a[1].number == 20);
    assert(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_object_create(c, &object) == FVM_OK);
    assert(fvm_object_set(c, object, "first", &a[0]) == FVM_OK);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = object;
    return FVM_OK;
}
static FVM_Result primitive(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a,
                            FVM_Value *out, void *user)
{
    (void)c;
    (void)self;
    (void)n;
    (void)a;
    (void)user;
    fvm_value_number(out, 1);
    return FVM_OK;
}
int main(void)
{
    static const unsigned char code[] = {
        0x96, 10, 0, 0,    'i',  'n', 's', 't', 'a', 'n', 'c', 'e', 0,    0x96, 5,    0,    7,
        20,   0,  0, 0,    0x96, 5,   0,   7,   10,  0,   0,   0,   0x96, 5,    0,    7,    2,
        0,    0,  0, 0x96, 8,    0,   0,   'N', 'a', 't', 'i', 'v', 'e',  0,    0x40, 0x1d, 0};
    static const unsigned char underflow[] = {0x96, 5,   0,   7,   3,   0,   0,   0, 0x96, 8, 0,
                                              0,    'N', 'a', 't', 'i', 'v', 'e', 0, 0x40, 0};
    static const unsigned char negative[] = {0x96, 5,   0,   7,   255, 255, 255, 255, 0x96, 8, 0,
                                             0,    'N', 'a', 't', 'i', 'v', 'e', 0,   0x40, 0};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *prototype = 0, *accessor_root = 0, *ctor_root = 0, *object = 0;
    FVM_Value value, out;
    int own;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_object_create(c, &receiver) == FVM_OK);
    assert(fvm_object_create(c, &prototype) == FVM_OK);
    assert(fvm_object_create(c, &accessor_root) == FVM_OK);
    assert(fvm_userdata_attach(c, accessor_root, &released, release) == FVM_OK);
    assert(fvm_object_set_prototype(c, receiver, prototype) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = receiver;
    assert(fvm_set_global(c, "receiver", &value) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, prototype, "live", getter, setter, accessor_root,
                                             accessor_root) == FVM_OK);
    assert(fvm_object_has_own(c, prototype, "live", &own) == FVM_OK && own);
    assert(fvm_object_has_own(c, receiver, "live", &own) == FVM_OK && !own);
    fvm_value_number(&value, 42);
    assert(fvm_object_set(c, receiver, "live", &value) == FVM_OK);
    assert(fvm_object_get(c, receiver, "live", &out) == FVM_OK && out.number == 42);
    assert(fvm_object_define_accessor_rooted(c, receiver, "readonly", getter, 0, accessor_root,
                                             accessor_root) == FVM_OK);
    assert(fvm_object_set(c, receiver, "readonly", &value) == FVM_OK && stored == 42);
    assert(fvm_object_get(c, receiver, "readonly", &out) == FVM_OK && out.number == 42);
    assert(fvm_object_define_accessor_rooted(c, receiver, "writeonly", 0, setter, accessor_root,
                                             accessor_root) == FVM_OK);
    assert(fvm_object_get(c, receiver, "writeonly", &out) == FVM_OK &&
           out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_set(c, receiver, "data", &value) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, receiver, "data", getter, setter, accessor_root,
                                             accessor_root) == FVM_INVALID_ARGUMENT);
    assert(fvm_object_define_accessor_rooted(other, receiver, "bad", getter, setter, 0, 0) ==
           FVM_INVALID_ARGUMENT);
    assert(fvm_object_create(c, &ctor_root) == FVM_OK);
    assert(fvm_userdata_attach(c, ctor_root, &released, release) == FVM_OK);
    assert(fvm_register_constructor_rooted(c, "Native", constructor, ctor_root, ctor_root) ==
           FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 0);
    assert(fvm_gc_mode(c, 100) == FVM_OK);
    {
        unsigned int i;
        for (i = 0; i < 105; ++i)
            assert(fvm_object_create(c, &object) == FVM_OK);
    }
    assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
    assert(fvm_get_global(c, "instance", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, out.object, "first", &value) == FVM_OK && value.number == 10);
    assert(fvm_register_constructor_rooted(c, "Native", primitive, 0, 0) == FVM_OK);
    assert(fvm_construct(c, "Native", 0, 0, &object) == FVM_ERROR && !object);
    fvm_value_undefined(&value);
    value.type = (FVM_ValueType)99;
    assert(fvm_construct(c, "Native", 1, &value, &object) == FVM_INVALID_ARGUMENT && !object);
    assert(fvm_object_set(c, receiver, "data", &value) == FVM_INVALID_ARGUMENT);
    assert(fvm_construct(c, "Missing", 0, 0, &object) == FVM_UNSUPPORTED && !object);
    assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && object);
    assert(fvm_register_constructor_rooted(c, "Object", primitive, 0, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c, underflow, sizeof(underflow)) == FVM_ERROR);
    assert(fvm_avm1_execute(c, negative, sizeof(negative)) == FVM_ERROR);
    assert(fvm_register_function(c, "FunctionOnly", primitive, 0) == FVM_OK);
    assert(fvm_construct(c, "FunctionOnly", 0, 0, &object) == FVM_UNSUPPORTED && !object);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 1);
    assert(fvm_object_define_accessor_rooted(c, prototype, "live", primitive, 0, 0, 0) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, receiver, "readonly", primitive, 0, 0, 0) ==
           FVM_OK);
    assert(fvm_object_define_accessor_rooted(c, receiver, "writeonly", primitive, 0, 0, 0) ==
           FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 2);
    assert(fvm_object_get(c, receiver, "live", &out) == FVM_OK && out.number == 1);
    fvm_value_undefined(&value);
    assert(fvm_set_global(c, "receiver", &value) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK && released == 2);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
