/* C89 writes select bindings without invoking getters and distinguish path/literal names. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/property.h"

static unsigned writes;
static FVM_Object *expected_receiver;


static FVM_Result forbidden(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    (void)c;
    (void)o;
    (void)n;
    (void)a;
    (void)out;
    (void)raw;
    if (!(0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static FVM_Result setter(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    (void)c;
    (void)raw;
    if (!(o == expected_receiver && n == 1 && a[0].number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++writes;
    fvm_value_undefined(out);
    return FVM_OK;
}


static void object_value(FVM_Value *value, FVM_Object *object)
{
    fvm_value_undefined(value);
    value->type = FVM_VALUE_OBJECT;
    value->object = object;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *inner, *outer, *parent, *scope, *leaf;
    FVM_Value name, value, out;
    int own;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &inner) == FVM_OK && fvm_object_create(c, &outer) == FVM_OK &&
          fvm_object_create(c, &parent) == FVM_OK && fvm_object_create(c, &leaf) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, inner, parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&value, inner);
    if (!(fvm_exec_with_scope(c, outer, &value, &scope) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, parent, "value", forbidden, setter, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    expected_receiver = inner;
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, outer, "value", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 42);
    fvm_value_string(&name, "value");
    if (!(fvm_exec_variable_set(c, scope, 0, 0, &name, &value) == FVM_OK && writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, outer, "value", &out) == FVM_OK && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string_bytes(&name, "a\0b", 3);
    if (!(fvm_exec_variable_set(c, outer, 0, 0, &name, &value) == FVM_OK &&
          fvm_get_global_bytes(c, "a\0b", 3, &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&out, leaf);
    if (!(fvm_object_set(c, inner, "child", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "child.new");
    if (!(fvm_exec_variable_set(c, scope, 0, 0, &name, &value) == FVM_OK &&
          fvm_object_get(c, leaf, "new", &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "missing.new");
    if (!(fvm_exec_variable_set(c, scope, 0, 0, &name, &value) == FVM_OK &&
          fvm_object_has_own(c, outer, "missing.new", &own) == FVM_OK && !own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "literal/slash");
    if (!(fvm_exec_variable_set(c, scope, outer, 0, &name, &value) == FVM_OK &&
          fvm_object_get(c, outer, "literal/slash", &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "");
    if (!(fvm_exec_variable_set(c, scope, outer, 0, &name, &value) == FVM_OK &&
          fvm_object_has_own(c, outer, "", &own) == FVM_OK && !own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&name, 1);
    if (!(fvm_exec_variable_set(c, scope, 0, 0, &name, &value) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
