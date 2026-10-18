#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <math.h>

static double invoke(FVM_Context *c, FVM_Object *math, const char *name, unsigned int n, double x,
                     double y)
{
    FVM_Value args[2], out;
    fvm_value_number(args, x);
    fvm_value_number(args + 1, y);
    assert(fvm_object_call(c, math, name, n, args, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_NUMBER);
    return out.number;
}
int main(void)
{
    static const unsigned char actions[] = {0x96, 8, 0, 0, 'a', 'n', 's', 'w', 'e',  'r',  0, 0x96,
                                            21,   0, 7, 3, 0,   0,   0,   7,   2,    0,    0, 0,
                                            7,    2, 0, 0, 0,   0,   'M', 'a', 't',  'h',  0, 0x1c,
                                            0x96, 5, 0, 0, 'p', 'o', 'w', 0,   0x52, 0x1d, 0};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *math;
    FVM_Value out, args[2], v;
    double inf, nan, number;
    unsigned int i;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "Math", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    math = out.object;
    assert(fvm_get_global(c, "Infinity", &out) == FVM_OK);
    inf = out.number;
    assert(fvm_get_global(c, "NaN", &out) == FVM_OK);
    nan = out.number;
    assert(nan != nan);
    assert(fvm_object_get(c, math, "PI", &out) == FVM_OK &&
           fabs(out.number - 3.141592653589793) < 1e-15);
    assert(invoke(c, math, "abs", 1, -3, 0) == 3);
    assert(invoke(c, math, "acos", 1, 1, 0) == 0);
    assert(invoke(c, math, "asin", 1, 0, 0) == 0);
    assert(invoke(c, math, "atan", 1, 0, 0) == 0);
    assert(invoke(c, math, "atan2", 2, 0, 1) == 0);
    assert(invoke(c, math, "ceil", 1, 1.2, 0) == 2);
    assert(invoke(c, math, "cos", 1, 0, 0) == 1);
    assert(invoke(c, math, "exp", 1, 0, 0) == 1);
    assert(invoke(c, math, "floor", 1, -1.2, 0) == -2);
    assert(invoke(c, math, "log", 1, 1, 0) == 0);
    assert(invoke(c, math, "max", 2, 3, 4) == 4);
    assert(invoke(c, math, "min", 2, 3, 4) == 3);
    assert(invoke(c, math, "pow", 2, 2, 3) == 8);
    assert(invoke(c, math, "sin", 1, 0, 0) == 0);
    assert(invoke(c, math, "sqrt", 1, 9, 0) == 3);
    assert(invoke(c, math, "tan", 1, 0, 0) == 0);
    assert(invoke(c, math, "round", 1, -1.5, 0) == -1);
    assert(invoke(c, math, "round", 1, 0.49999999999999994, 0) == 0);
    assert(1 / invoke(c, math, "round", 1, -0.5, 0) == -inf);
    assert(1 / invoke(c, math, "max", 2, -0.0, 0.0) == inf);
    assert(1 / invoke(c, math, "min", 2, 0.0, -0.0) == -inf);
    assert(invoke(c, math, "min", 0, 0, 0) == inf);
    assert(invoke(c, math, "max", 0, 0, 0) == -inf);
    number = invoke(c, math, "sqrt", 1, -1, 0);
    assert(number != number);
    number = invoke(c, math, "abs", 0, 0, 0);
    assert(number != number);
    number = invoke(c, math, "max", 2, nan, 1);
    assert(number != number);
    assert(invoke(c, math, "pow", 2, nan, 0) == 1);
    number = invoke(c, math, "pow", 2, 1, inf);
    assert(number != number);
    for (i = 0; i < 1000; ++i)
    {
        number = invoke(c, math, "random", 0, 0, 0);
        assert(number >= 0 && number < 1);
    }
    fvm_value_string(&v, "  -1.25e2 \t");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && number == -125);
    fvm_value_string(&v, "0x2a");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && number == 42);
    fvm_value_string(&v, "42junk");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && number != number);
    fvm_value_string(&v, "1e");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && number != number);
    fvm_value_string(&v, "1e-320");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && number > 0 && number < 1e-319);
    fvm_value_string(&v, " ");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && number == 0);
    fvm_value_string(&v, "-0");
    assert(fvm_value_to_number(c, &v, &number) == FVM_OK && 1 / number == -inf);
    assert(fvm_call(c, "Number", 0, 0, &out) == FVM_OK && out.number == 0);
    fvm_value_string(args, "12");
    assert(fvm_call(c, "Number", 1, args, &out) == FVM_OK && out.number == 12);
    fvm_value_number(args, nan);
    assert(fvm_call(c, "Boolean", 1, args, &out) == FVM_OK && !out.boolean);
    fvm_value_string(args, "false");
    assert(fvm_call(c, "Boolean", 1, args, &out) == FVM_OK && out.boolean);
    fvm_value_string(args, "nonsense");
    assert(fvm_call(c, "isNaN", 1, args, &out) == FVM_OK && out.boolean);
    fvm_value_number(args, inf);
    assert(fvm_call(c, "isFinite", 1, args, &out) == FVM_OK && !out.boolean);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK);
    assert(fvm_get_global(c, "answer", &out) == FVM_OK && out.number == 8);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    fvm_value_number(&v, 99);
    assert(fvm_set_global(other, "isFinite", &v) == FVM_OK);
    assert(fvm_standard_install(other) == FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(other, "Math", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_get_global(other, "isFinite", &out) == FVM_OK && out.number == 99);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
