#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include <assert.h>
#include <string.h>
#include <float.h>
#include <stdlib.h>
static void expect(FVM_Context *c, FVM_Value *value, const char *expected)
{
    FVM_Value out;
    assert(fvm_value_to_string(c, value, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_STRING && !strcmp(out.string, expected));
}
static FVM_Result self_value(FVM_Context *c, FVM_Object *self, unsigned int n,
                             const FVM_Value *args, FVM_Value *out, void *user)
{
    (void)c;
    (void)n;
    (void)args;
    (void)user;
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = self;
    return FVM_OK;
}
static FVM_Result numeric(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                          FVM_Value *out, void *user)
{
    (void)c;
    (void)self;
    (void)n;
    (void)args;
    (void)user;
    fvm_value_number(out, 73);
    return FVM_OK;
}
static FVM_Result raises(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                         FVM_Value *out, void *user)
{
    FVM_Value value;
    (void)self;
    (void)n;
    (void)args;
    (void)out;
    (void)user;
    fvm_value_string(&value, "conversion threw");
    return fvm_exception_raise(c, &value);
}
static FVM_Result recursive(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                            FVM_Value *out, void *user)
{
    FVM_Value value;
    (void)n;
    (void)args;
    (void)user;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = self;
    return fvm_value_to_string(c, &value, out);
}
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Value value, out;
    FVM_Object *object = 0, *foreign = 0;
    unsigned int i;
    char temporary[] = "copied buffer";
    const double numbers[] = {0, -0.0, 0.1, -42.5, 1e-6, 1e-7, 1e20, 1e21, DBL_MIN, DBL_MAX};
    const char *texts[] = {"0",
                           "0",
                           "0.1",
                           "-42.5",
                           "0.000001",
                           "1e-7",
                           "100000000000000000000",
                           "1e+21",
                           "2.2250738585072014e-308",
                           "1.7976931348623157e+308"};
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    fvm_value_undefined(&value);
    expect(c, &value, "undefined");
    value.type = FVM_VALUE_NULL;
    expect(c, &value, "null");
    fvm_value_boolean(&value, 0);
    expect(c, &value, "false");
    fvm_value_boolean(&value, 1);
    expect(c, &value, "true");
    for (i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i)
    {
        fvm_value_number(&value, numbers[i]);
        expect(c, &value, texts[i]);
        assert(fvm_value_to_string(c, &value, &out) == FVM_OK);
        assert(strtod(out.string, 0) == numbers[i]);
    }
    /* Deterministic binary64 samples exercise exponent/sign/digit boundaries.
       Skip this layout-dependent sample only on non-IEEE double platforms. */
    if (sizeof(double) == 8 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024)
    {
        unsigned long state = 89173UL;
        unsigned char bytes[8];
        unsigned int sample, byte;
        double number;
        for (sample = 0; sample < 256; ++sample)
        {
            for (byte = 0; byte < 8; ++byte)
            {
                state = (state * 1664525UL + 1013904223UL) & 0xffffffffUL;
                bytes[byte] = (unsigned char)(state >> 24);
            }
            memcpy(&number, bytes, sizeof(number));
            if (number != number || number > DBL_MAX || number < -DBL_MAX)
                continue;
            fvm_value_number(&value, number);
            assert(fvm_value_to_string(c, &value, &out) == FVM_OK);
            assert(strtod(out.string, 0) == number);
        }
    }
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "NaN", &value) == FVM_OK);
    expect(c, &value, "NaN");
    assert(fvm_get_global(c, "Infinity", &value) == FVM_OK);
    expect(c, &value, "Infinity");
    value.number = -value.number;
    expect(c, &value, "-Infinity");
    fvm_value_string(&value, "");
    expect(c, &value, "");
    fvm_value_string(&value, temporary);
    assert(fvm_value_to_string(c, &value, &out) == FVM_OK);
    temporary[0] = 'X';
    assert(!strcmp(out.string, "copied buffer"));
    fvm_value_number(&value, 17);
    assert(fvm_value_to_string(c, &value, &value) == FVM_OK);
    assert(!strcmp(value.string, "17"));
    assert(fvm_object_create(c, &object) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    expect(c, &value, "[object Object]");
    assert(fvm_object_define_method_rooted(c, object, "toString", self_value, 0, 0) == FVM_OK);
    assert(fvm_value_to_string(c, &value, &out) == FVM_UNSUPPORTED &&
           out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_object_define_method_rooted(c, object, "valueOf", numeric, 0, 0) == FVM_OK);
    expect(c, &value, "73");
    assert(fvm_object_define_method_rooted(c, object, "toString", raises, 0, 0) == FVM_OK);
    assert(fvm_value_to_string(c, &value, &out) == FVM_EXCEPTION);
    assert(fvm_exception_get(c, &out) == FVM_OK && !strcmp(out.string, "conversion threw"));
    assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, object, "toString", recursive, 0, 0) == FVM_OK);
    assert(fvm_value_to_string(c, &value, &out) == FVM_UNSUPPORTED);
    /* Recursion unwinding restores the conversion depth for later calls. */
    fvm_value_number(&value, 8);
    expect(c, &value, "8");
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_object_create(other, &foreign) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = foreign;
    assert(fvm_value_to_string(c, &value, &out) == FVM_INVALID_ARGUMENT);
    fvm_value_undefined(&value);
    value.type = (FVM_ValueType)99;
    assert(fvm_value_to_string(c, &value, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_value_to_string(c, 0, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_value_to_string(0, &value, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_value_to_string(c, &value, 0) == FVM_INVALID_ARGUMENT);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
