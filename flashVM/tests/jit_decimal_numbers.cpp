/* Number, ToNumber and numeric actions share exact legacy decimal results in native code. */
#define FVM_DECIMAL_NUMBERS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_decimal_numbers.cpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            FVM_Context *c = 0;
            FVM_Value out;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            parsing_load(c, version);
            Program number;
            number.text(".1499999");
            number.number(1);
            number.text("Number");
            number.op(0x3d);
            number.finish();
            out = class_run(c, number);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program action;
            action.text(".1499999");
            action.op(0x4a);
            action.finish();
            out = class_run(c, action);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program subtract;
            subtract.text(".1499999");
            subtract.number(0);
            subtract.op(0x0b);
            subtract.finish();
            out = class_run(c, subtract);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 0.14999990000000005))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program malformed;
            malformed.text("1.2.5");
            malformed.op(0x4a);
            malformed.finish();
            out = class_run(c, malformed);
            if (!(out.type == FVM_VALUE_NUMBER))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(version == 4 ? out.number == 1.25 : out.number != out.number))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program wrapped;
            wrapped.text("1e4294967297");
            wrapped.op(0x4a);
            wrapped.finish();
            out = class_run(c, wrapped);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 10))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
