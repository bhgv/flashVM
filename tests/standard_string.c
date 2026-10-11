#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>
static void text(FVM_Context *c, FVM_Object *s, const char *method, unsigned int count,
                 FVM_Value *args, const char *expected)
{
    FVM_Value out;
    assert(fvm_object_call(c, s, method, count, args, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_STRING && !strcmp(out.string, expected));
}
static void number(FVM_Context *c, FVM_Object *s, const char *method, unsigned int count,
                   FVM_Value *args, double expected)
{
    FVM_Value out;
    assert(fvm_object_call(c, s, method, count, args, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_NUMBER && out.number == expected);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *s = 0, *empty = 0;
    FVM_Value input, args[3], out, klass, copy, ctor;
    FVM_Channel *channel = 0;
    double primitive;
    int matches;
    const char *sample =
        "A\320\221\360\237\230\200Z"; /* A, Cyrillic Be, emoji, Z: five UTF-16 units. */
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "String", &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT);
    assert(fvm_construct(c, "String", 0, 0, &empty) == FVM_OK);
    text(c, empty, "valueOf", 0, 0, "");
    fvm_value_string(&input, sample);
    assert(fvm_construct(c, "String", 1, &input, &s) == FVM_OK);
    assert(fvm_object_get(c, s, "length", &out) == FVM_OK && out.number == 5);
    fvm_value_number(&out, 99);
    assert(fvm_object_set(c, s, "length", &out) == FVM_OK);
    assert(fvm_object_get(c, s, "length", &out) == FVM_OK && out.number == 5);
    text(c, s, "toString", 0, 0, sample);
    fvm_value_number(args, 1);
    text(c, s, "charAt", 1, args, "\320\221");
    number(c, s, "charCodeAt", 1, args, 1041);
    fvm_value_number(args, 2);
    number(c, s, "charCodeAt", 1, args, 0xd83d);
    assert(fvm_object_call(c, s, "charAt", 1, args, &out) == FVM_UNSUPPORTED);
    fvm_value_number(args, 3);
    number(c, s, "charCodeAt", 1, args, 0xde00);
    fvm_value_number(args, 99);
    text(c, s, "charAt", 1, args, "");
    assert(fvm_object_call(c, s, "charCodeAt", 1, args, &out) == FVM_OK &&
           out.number != out.number);
    fvm_value_number(args, 2);
    fvm_value_number(args + 1, 4);
    text(c, s, "substring", 2, args, "\360\237\230\200");
    fvm_value_number(args, 4);
    fvm_value_number(args + 1, 1);
    text(c, s, "substring", 2, args, "\320\221\360\237\230\200");
    fvm_value_number(args, -3);
    fvm_value_number(args + 1, -1);
    text(c, s, "slice", 2, args, "\360\237\230\200");
    fvm_value_number(args, -3);
    fvm_value_number(args + 1, 2);
    text(c, s, "substr", 2, args, "\360\237\230\200");
    fvm_value_number(args, 4);
    fvm_value_undefined(args + 1);
    text(c, s, "slice", 2, args, "Z");
    text(c, s, "substr", 2, args, "Z");
    text(c, s, "substring", 2, args, "Z");
    fvm_value_string(args, "Z");
    number(c, s, "indexOf", 1, args, 4);
    fvm_value_string(args, "\360\237\230\200");
    number(c, s, "lastIndexOf", 1, args, 2);
    fvm_value_string(args, "missing");
    number(c, s, "lastIndexOf", 1, args, -1);
    fvm_value_string(args, "");
    number(c, s, "lastIndexOf", 1, args, 5);
    fvm_value_string(args, "!");
    text(c, s, "concat", 1, args, "A\320\221\360\237\230\200Z!");
    fvm_value_number(args, 65);
    fvm_value_number(args + 1, 0xd83d);
    fvm_value_number(args + 2, 0xde00);
    assert(fvm_object_call(c, klass.object, "fromCharCode", 3, args, &out) == FVM_OK &&
           !strcmp(out.string, "A\360\237\230\200"));
    fvm_value_number(args, 0);
    assert(fvm_object_call(c, klass.object, "fromCharCode", 1, args, &out) == FVM_UNSUPPORTED);
    fvm_value_number(args, 0xd800);
    assert(fvm_object_call(c, klass.object, "fromCharCode", 1, args, &out) == FVM_UNSUPPORTED);
    fvm_value_undefined(&input);
    input.type = FVM_VALUE_OBJECT;
    input.object = s;
    assert(fvm_value_instance_of(c, &input, &klass, &matches) == FVM_OK && matches);
    assert(fvm_object_set(c, s, "self", &input) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &input, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_gc_pin(other, copy.object) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    text(other, copy.object, "valueOf", 0, 0, sample);
    assert(fvm_object_get(other, copy.object, "length", &out) == FVM_OK && out.number == 5);
    assert(fvm_object_get(other, copy.object, "constructor", &ctor) == FVM_OK);
    assert(fvm_value_instance_of(other, &copy, &ctor, &matches) == FVM_OK && matches);
    assert(fvm_object_get(other, copy.object, "self", &out) == FVM_OK && out.object == copy.object);
    fvm_value_string(&input, "123.5");
    assert(fvm_construct(c, "String", 1, &input, &empty) == FVM_OK);
    input.type = FVM_VALUE_OBJECT;
    input.object = empty;
    assert(fvm_value_to_number(c, &input, &primitive) == FVM_OK && primitive == 123.5);
    fvm_value_string(&input, "\300\200");
    assert(fvm_construct(c, "String", 1, &input, &empty) == FVM_INVALID_ARGUMENT && !empty);
    fvm_value_undefined(&input);
    assert(fvm_set_global(c, "String", &input) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_construct(c, "String", 0, 0, &empty) == FVM_OK);
    text(c, empty, "valueOf", 0, 0, "");
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
