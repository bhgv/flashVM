#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <string.h>
#include <float.h>
static void format(FVM_Context *c, FVM_Object *number, double radix, const char *expected)
{
    FVM_Value arg, out;
    fvm_value_number(&arg, radix);
    assert(fvm_object_call(c, number, "toString", 1, &arg, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_STRING && !strcmp(out.string, expected));
}
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *number = 0, *zero = 0;
    FVM_Channel *channel = 0;
    FVM_Value arg, value, klass, prototype, copy, ctor;
    double unwrapped;
    int matches;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "Number", &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, klass.object, "MAX_VALUE", &value) == FVM_OK &&
           value.number == DBL_MAX);
    assert(fvm_object_get(c, klass.object, "MIN_VALUE", &value) == FVM_OK && value.number > 0 &&
           value.number <= DBL_MIN);
    assert(fvm_object_get(c, klass.object, "NaN", &value) == FVM_OK &&
           value.number != value.number);
    assert(fvm_object_get(c, klass.object, "prototype", &prototype) == FVM_OK);
    assert(fvm_object_call(c, prototype.object, "valueOf", 0, 0, &value) == FVM_OK &&
           value.number == 0);
    assert(fvm_construct(c, "Number", 0, 0, &zero) == FVM_OK);
    format(c, zero, 2, "0");
    fvm_value_string(&arg, "-255");
    assert(fvm_call(c, "Number", 1, &arg, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
           value.number == -255);
    assert(fvm_construct(c, "Number", 1, &arg, &number) == FVM_OK);
    format(c, number, 16, "-ff");
    format(c, number, 10, "-255");
    value.type = FVM_VALUE_OBJECT;
    value.object = number;
    assert(fvm_value_to_number(c, &value, &unwrapped) == FVM_OK && unwrapped == -255);
    assert(fvm_value_instance_of(c, &value, &klass, &matches) == FVM_OK && matches);
    assert(fvm_object_set(c, number, "cycle", &value) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_set_global(other, "copy", &copy) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    format(other, copy.object, 16, "-ff");
    assert(fvm_object_get(other, copy.object, "constructor", &ctor) == FVM_OK);
    assert(fvm_value_instance_of(other, &copy, &ctor, &matches) == FVM_OK && matches);
    assert(fvm_object_get(other, copy.object, "cycle", &arg) == FVM_OK &&
           arg.object == copy.object);
    fvm_value_number(&arg, 1);
    assert(fvm_object_call(c, number, "toString", 1, &arg, &value) == FVM_INVALID_ARGUMENT);
    fvm_value_number(&arg, 0.25);
    assert(fvm_construct(c, "Number", 1, &arg, &zero) == FVM_OK);
    fvm_value_number(&arg, 2);
    assert(fvm_object_call(c, zero, "toString", 1, &arg, &value) == FVM_UNSUPPORTED);
    assert(fvm_object_call(c, zero, "toString", 0, 0, &value) == FVM_OK &&
           !strcmp(value.string, "0.25"));
    fvm_value_undefined(&arg);
    assert(fvm_construct(c, "Number", 1, &arg, &zero) == FVM_OK);
    assert(fvm_object_call(c, zero, "valueOf", 0, 0, &value) == FVM_OK &&
           value.number != value.number);
    fvm_value_number(&arg, 9007199254740991.0);
    assert(fvm_construct(c, "Number", 1, &arg, &zero) == FVM_OK);
    format(c, zero, 16, "1fffffffffffff");
    format(c, zero, 36, "2gosa7pa2gv");
    fvm_value_number(&arg, -0.0);
    assert(fvm_construct(c, "Number", 1, &arg, &zero) == FVM_OK);
    format(c, zero, 2, "0");
    assert(fvm_object_get(c, klass.object, "POSITIVE_INFINITY", &arg) == FVM_OK &&
           arg.number > DBL_MAX);
    assert(fvm_construct(c, "Number", 1, &arg, &zero) == FVM_OK);
    format(c, zero, 2, "Infinity");
    fvm_value_undefined(&value);
    assert(fvm_set_global(c, "Number", &value) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_construct(c, "Number", 0, 0, &zero) == FVM_OK);
    format(c, zero, 10, "0");
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
