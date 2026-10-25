/* Native script number conversion retains inputs through collection and exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include <limits>


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *o = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned id = 2;
    attach(c, o, "valueOf", numeric, &id);
    FVM_Value value, out;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
    calls = 0;
    if (!(fvm_exec_to_number(c, 0, &value, &out) == FVM_OK && out.number == 2 && calls == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    calls = 0;
    failing = true;
    fvm_value_number(&out, 99);
    if (!(fvm_exec_to_number(c, 1, &value, &out) == FVM_EXCEPTION && out.number == 99 && calls == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failing = false;
    fvm_value_number(&value, std::numeric_limits<double>::infinity());
    if (!(fvm_exec_to_number(c, 1, &value, &out) == FVM_OK && out.number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, std::numeric_limits<double>::quiet_NaN());
    if (!(fvm_exec_to_number(c, 1, &value, &out) == FVM_OK && out.number == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, -0.0);
    if (!(fvm_exec_to_number(c, 1, &value, &out) == FVM_OK && out.number == 0 &&
          1.0 / out.number == std::numeric_limits<double>::infinity()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
