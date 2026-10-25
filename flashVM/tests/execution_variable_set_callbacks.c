/* Assignment roots values through script setter GC, self-removal, exceptions and outer barriers. */
#define FVM_SETTER_COLLECTOR_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "script_setter_collector.c"
#include "flashvm/execution.h"


static FVM_Result
assignment_host(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    FVM_Value name;
    FVM_Result r;
    (void)raw;
    if (!(n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "child.value");
    in_host = 1;
    r = fvm_exec_variable_set(c, self, self, 0, &name, args);
    in_host = 0;
    fvm_value_undefined(out);
    return r;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *scope, *owner, *plain, *capture, *fn, *input, *host_fn;
    FVM_Value name, value, out;
    const unsigned char code[] = {0};
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &scope) == FVM_OK && fvm_gc_pin(c, scope) == FVM_OK &&
          fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK &&
          fvm_object_create(c, &capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 99);
    if (!(fvm_object_set(c, capture, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, setter, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = owner;
    if (!(fvm_object_set(c, scope, "child", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 11);
    if (!(fvm_object_set(c, owner, "value", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    install(c, owner, plain, fn);
    if (!(fvm_object_create(c, &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, input, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value.type = FVM_VALUE_OBJECT;
    value.object = input;
    fvm_value_string(&name, "child.value");
    if (!(fvm_exec_variable_set(c, scope, scope, 0, &name, &value) == FVM_OK &&
          fvm_object_get(c, owner, "value", &out) == FVM_OK && out.object == input))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 2;
    fvm_value_number(&value, 17);
    if (!(fvm_exec_variable_set(c, scope, scope, 0, &name, &value) == FVM_EXCEPTION &&
          fvm_exception_clear(c) == FVM_OK && fvm_object_get(c, owner, "value", &out) == FVM_OK && out.object == input))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 1;
    if (!(fvm_exec_variable_set(c, scope, scope, 0, &name, &value) == FVM_OK &&
          fvm_object_get(c, owner, "value", &out) == FVM_OK && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 0;
    install(c, owner, plain, fn);
    if (!(fvm_function_create_native(c, assignment_host, 0, 0, &host_fn) == FVM_OK &&
          fvm_function_invoke(c, host_fn, scope, 1, &value, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
