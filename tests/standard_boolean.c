#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>

static void check(FVM_Context *c, const FVM_Value *input, int truth)
{
    FVM_Object *object = 0;
    FVM_Value out, boxed;
    double number = -1;
    assert(fvm_construct(c, "Boolean", input ? 1 : 0, input, &object) == FVM_OK);
    assert(fvm_gc_pin(c, object) == FVM_OK);
    assert(fvm_object_call(c, object, "valueOf", 0, 0, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_BOOLEAN && out.boolean == truth);
    assert(fvm_object_call(c, object, "toString", 0, 0, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_STRING && strcmp(out.string, truth ? "true" : "false") == 0);
    fvm_value_undefined(&boxed);
    boxed.type = FVM_VALUE_OBJECT;
    boxed.object = object;
    assert(fvm_value_to_number(c, &boxed, &number) == FVM_OK && number == truth);
    /* A wrapper object is truthy even when its stored primitive is false. */
    assert(fvm_call(c, "Boolean", 1, &boxed, &out) == FVM_OK && out.boolean);
    assert(fvm_call(c, "Number", 1, &boxed, &out) == FVM_OK && out.number == truth);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, object, "valueOf", 0, 0, &out) == FVM_OK && out.boolean == truth);
    assert(fvm_gc_unpin(c, object) == FVM_OK);
}
int main(void)
{
    /* boxed = new Boolean(false); value = boxed.valueOf(); text = boxed.toString(). */
    static const unsigned char actions[] = {
        0x96, 7,    0,    0,    'b',  'o',  'x', 'e',  'd', 0,    0x96, 2,    0,    5,   0,    0x96,
        5,    0,    7,    1,    0,    0,    0,   0x96, 9,   0,    0,    'B',  'o',  'o', 'l',  'e',
        'a',  'n',  0,    0x40, 0x1d, 0x96, 7,   0,    0,   'v',  'a',  'l',  'u',  'e', 0,    0x96,
        5,    0,    7,    0,    0,    0,    0,   0x96, 7,   0,    0,    'b',  'o',  'x', 'e',  'd',
        0,    0x1c, 0x96, 9,    0,    0,    'v', 'a',  'l', 'u',  'e',  'O',  'f',  0,   0x52, 0x1d,
        0x96, 6,    0,    0,    't',  'e',  'x', 't',  0,   0x96, 5,    0,    7,    0,   0,    0,
        0,    0x96, 7,    0,    0,    'b',  'o', 'x',  'e', 'd',  0,    0x1c, 0x96, 10,  0,    0,
        't',  'o',  'S',  't',  'r',  'i',  'n', 'g',  0,   0x52, 0x1d, 0};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *object = 0, *prototype = 0, *plain = 0, *foreign = 0;
    FVM_Value v, out, klass, args[2];
    FVM_Channel *channel = 0;
    double number = 123;
    int own;
    unsigned int i;
    unsigned long reclaimed;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_construct(c, "Boolean", 0, 0, &object) == FVM_UNSUPPORTED);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    check(c, 0, 0);
    fvm_value_undefined(&v);
    check(c, &v, 0);
    v.type = FVM_VALUE_NULL;
    check(c, &v, 0);
    fvm_value_boolean(&v, 0);
    check(c, &v, 0);
    fvm_value_boolean(&v, 1);
    check(c, &v, 1);
    fvm_value_number(&v, 0);
    check(c, &v, 0);
    fvm_value_number(&v, -0.0);
    check(c, &v, 0);
    fvm_value_number(&v, -2);
    check(c, &v, 1);
    assert(fvm_get_global(c, "NaN", &v) == FVM_OK);
    check(c, &v, 0);
    assert(fvm_get_global(c, "Infinity", &v) == FVM_OK);
    check(c, &v, 1);
    assert(fvm_value_string(&v, "") == FVM_OK);
    check(c, &v, 0);
    assert(fvm_value_string(&v, "false") == FVM_OK);
    check(c, &v, 1);
    assert(fvm_object_create(c, &plain) == FVM_OK);
    assert(fvm_gc_pin(c, plain) == FVM_OK);
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = plain;
    check(c, &v, 1);
    assert(fvm_get_global(c, "Boolean", &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, klass.object, "prototype", &out) == FVM_OK);
    prototype = out.object;
    assert(fvm_object_get(c, prototype, "constructor", &out) == FVM_OK &&
           out.object == klass.object);
    assert(fvm_object_call(c, prototype, "valueOf", 0, 0, &out) == FVM_OK && !out.boolean);
    assert(fvm_object_has_own(c, prototype, "valueOf", &own) == FVM_OK && own);
    assert(fvm_construct(c, "Boolean", 0, 0, &object) == FVM_OK);
    assert(fvm_gc_pin(c, object) == FVM_OK);
    assert(fvm_object_get_prototype(c, object, &prototype) == FVM_OK && prototype);
    assert(fvm_object_has_own(c, object, "valueOf", &own) == FVM_OK && !own);
    fvm_value_boolean(args, 0);
    fvm_value_boolean(args + 1, 1);
    assert(fvm_object_call(c, object, "valueOf", 2, args, &out) == FVM_OK && !out.boolean);
    assert(fvm_construct(c, "Boolean", 2, args, &foreign) == FVM_OK);
    assert(fvm_object_call(c, foreign, "valueOf", 0, 0, &out) == FVM_OK && !out.boolean);
    assert(fvm_object_set_prototype(c, plain, prototype) == FVM_OK);
    assert(fvm_object_call(c, plain, "valueOf", 0, 0, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_object_set_prototype(c, plain, 0) == FVM_OK);
    fvm_value_boolean(&v, 1);
    assert(fvm_object_set(c, object, "wrapped", &v) == FVM_OK);
    assert(fvm_object_call(c, object, "valueOf", 0, 0, &out) == FVM_OK && !out.boolean);
    /* Constructor capture survives removing the global class metadata. */
    fvm_value_undefined(&v);
    assert(fvm_set_global(c, "Boolean", &v) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_construct(c, "Boolean", 0, 0, &foreign) == FVM_OK);
    assert(fvm_set_global(c, "Boolean", &klass) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_gc_mode(c, 100) == FVM_OK);
    for (i = 0; i < 105; ++i)
        assert(fvm_object_create(c, &foreign) == FVM_OK);
    assert(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK);
    assert(fvm_get_global(c, "value", &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN &&
           !out.boolean);
    assert(fvm_get_global(c, "text", &out) == FVM_OK && strcmp(out.string, "false") == 0);
    assert(fvm_get_global(c, "boxed", &v) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &v, 0) == FVM_UNSUPPORTED);
    fvm_channel_destroy(channel);
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_object_create(other, &foreign) == FVM_OK);
    v.object = foreign;
    assert(fvm_construct(c, "Boolean", 1, &v, &foreign) == FVM_INVALID_ARGUMENT);
    fvm_value_undefined(&v);
    v.type = (FVM_ValueType)99;
    assert(fvm_value_to_number(c, &v, &number) == FVM_INVALID_ARGUMENT && number == 123);
    fvm_value_number(&v, 99);
    assert(fvm_set_global(other, "Boolean", &v) == FVM_OK);
    assert(fvm_standard_install(other) == FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(other, "Math", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_get_global(other, "Boolean", &out) == FVM_OK && out.number == 99);
    assert(fvm_gc_unpin(c, plain) == FVM_OK);
    assert(fvm_gc_unpin(c, object) == FVM_OK);
    fvm_value_undefined(&v);
    assert(fvm_set_global(c, "boxed", &v) == FVM_OK);
    assert(fvm_gc_collect(c, &reclaimed) == FVM_OK && reclaimed >= 2);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
