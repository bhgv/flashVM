#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <assert.h>
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *ctor = 0, *found = 0;
    FVM_Value klass, args[2], out, copy;
    FVM_Channel *channel = 0;
    const unsigned char code[] = {0x96, 5, 0, 7, 11, 0, 0, 0, 0x3e, 0};
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "Object", &klass) == FVM_OK);
    assert(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &ctor) == FVM_OK);
    fvm_value_string(args, "Symbol");
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_OBJECT;
    args[1].object = ctor;
    assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK &&
           out.boolean);
    assert(fvm_registered_class(c, "Symbol", &found) == FVM_OK && found == ctor);
    assert(fvm_value_string_bytes(args, "s\0x", 3) == FVM_OK);
    assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean);
    assert(fvm_registered_class_bytes(c, "s\0x", 3, &found) == FVM_OK && found == ctor);
    fvm_value_string(args, "\xed\xa0\x80\xed\xb0\x80");
    assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK && out.boolean);
    assert(fvm_registered_class(c, "\xf0\x90\x80\x80", &found) == FVM_OK && found == ctor);
    fvm_value_string(args, "Symbol");
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_registered_class(c, "Symbol", &found) == FVM_OK && found == ctor);
    assert(fvm_function_invoke(c, found, 0, 0, 0, &out) == FVM_OK && out.number == 11);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_set_global(other, "Object", &copy) == FVM_OK);
    assert(fvm_registered_class(other, "Symbol", &found) == FVM_OK);
    assert(fvm_registered_class_bytes(other, "s\0x", 3, &found) == FVM_OK);
    assert(fvm_function_invoke(other, found, 0, 0, 0, &out) == FVM_OK && out.number == 11);
    assert(fvm_registered_class(other, "\xf0\x90\x80\x80", &found) == FVM_OK);
    assert(fvm_function_invoke(other, found, 0, 0, 0, &out) == FVM_OK && out.number == 11);
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_NULL;
    assert(fvm_object_call(other, copy.object, "registerClass", 2, args, &out) == FVM_OK &&
           out.boolean);
    assert(fvm_registered_class(other, "Symbol", &found) == FVM_NOT_FOUND);
    assert(fvm_registered_class(c, "Symbol", &found) == FVM_OK && found == ctor);
    fvm_value_number(args + 1, 42);
    assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK &&
           !out.boolean);
    assert(fvm_registered_class(c, "Symbol", &found) == FVM_OK && found == ctor);
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_OK &&
           out.boolean);
    assert(fvm_registered_class(c, "Symbol", &found) == FVM_NOT_FOUND);
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
