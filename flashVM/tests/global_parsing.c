#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include <assert.h>
#include <string.h>
#include <float.h>
static double parse(FVM_Context *c, const char *name, const char *text, double radix, int use_radix)
{
    FVM_Value args[2], out;
    fvm_value_string(args, text);
    fvm_value_number(args + 1, radix);
    assert(fvm_call(c, name, use_radix ? 2 : 1, args, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_NUMBER);
    return out.number;
}
static FVM_Result throws(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *args,
                         FVM_Value *out, void *userdata)
{
    FVM_Value error;
    (void)self;
    (void)n;
    (void)args;
    (void)out;
    (void)userdata;
    fvm_value_string(&error, "conversion failed");
    return fvm_exception_raise(c, &error);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *conflict = 0;
    FVM_Value value, out;
    FVM_Object *object = 0;
    double number;
    int radix, sample;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(parse(c, "parseInt", "0x3F8", 0, 0) == 1016);
    assert(parse(c, "parseInt", "0777", 0, 0) == 511);
    assert(parse(c, "parseInt", "0777", 10, 1) == 777);
    assert(parse(c, "parseInt", "08", 0, 0) == 0);
    assert(parse(c, "parseInt", "  -1010tail", 2, 1) == -10);
    assert(parse(c, "parseInt", "3E8", 16, 1) == 1000);
    assert(parse(c, "parseInt", "10", 4294967298.0, 1) == 2);
    assert(parse(c, "parseInt", "10", 2.9, 1) == 2);
    number = parse(c, "parseInt", "10", 1, 1);
    assert(number != number);
    number = parse(c, "parseInt", "0x", 16, 1);
    assert(number != number);
    number = parse(c, "parseInt", "words", 0, 0);
    assert(number != number);
    assert(parse(c, "parseInt", "\302\240\357\273\27742", 10, 1) == 42);
    assert(parse(c, "parseFloat", "-2.5tail", 0, 0) == -2.5);
    assert(parse(c, "parseFloat", "3.5e6 units", 0, 0) == 3500000);
    assert(parse(c, "parseFloat", "3.75math", 0, 0) == 3.75);
    assert(parse(c, "parseFloat", ".125e+2x", 0, 0) == 12.5);
    assert(parse(c, "parseFloat", "1e+tail", 0, 0) == 1);
    assert(parse(c, "parseFloat", "0x20", 0, 0) == 0);
    assert(parse(c, "parseFloat", "Infinitytail", 0, 0) > DBL_MAX);
    assert(parse(c, "parseFloat", "-Infinity!", 0, 0) < -DBL_MAX);
    assert(parse(c, "parseFloat", "\342\200\250 42.5", 0, 0) == 42.5);
    number = parse(c, "parseFloat", ".words", 0, 0);
    assert(number != number);
    assert(fvm_call(c, "parseInt", 0, 0, &out) == FVM_OK && out.number != out.number);
    assert(fvm_call(c, "parseFloat", 0, 0, &out) == FVM_OK && out.number != out.number);
    assert(fvm_call(c, "String", 0, 0, &out) == FVM_OK && !strcmp(out.string, ""));
    fvm_value_undefined(&value);
    assert(fvm_call(c, "String", 1, &value, &out) == FVM_OK && !strcmp(out.string, "undefined"));
    /* Independent integer encoder acts as an oracle for every supported base. */
    for (radix = 2; radix <= 36; ++radix)
        for (sample = 0; sample <= 200; ++sample)
        {
            char encoded[32];
            const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
            int n = sample, end = 31;
            encoded[end] = 0;
            do
            {
                encoded[--end] = digits[n % radix];
                n /= radix;
            } while (n);
            assert(parse(c, "parseInt", encoded + end, radix, 1) == sample);
        }
    assert(fvm_object_create(c, &object) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, object, "toString", throws, 0, 0) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    assert(fvm_call(c, "parseInt", 1, &value, &out) == FVM_EXCEPTION);
    assert(fvm_exception_get(c, &out) == FVM_OK && !strcmp(out.string, "conversion failed"));
    assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_call(c, "parseFloat", 1, &value, &out) == FVM_EXCEPTION);
    assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_call(c, "String", 1, &value, &out) == FVM_EXCEPTION);
    assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_context_create(rt, &conflict) == FVM_OK);
    fvm_value_number(&value, 9);
    assert(fvm_set_global(conflict, "parseInt", &value) == FVM_OK);
    assert(fvm_standard_install(conflict) == FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(conflict, "Math", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    fvm_context_destroy(conflict);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
