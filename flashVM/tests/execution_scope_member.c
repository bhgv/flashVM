/* C89 final-member presence stops retries even for undefined, null and primitive values. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/property.h"


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
    FVM_Object *inner, *outer, *bad, *good, *leaf, *scope;
    FVM_Value value, path, name, out, reference;
    int present, removed;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &inner) == FVM_OK && fvm_object_create(c, &outer) == FVM_OK &&
          fvm_object_create(c, &bad) == FVM_OK && fvm_object_create(c, &good) == FVM_OK &&
          fvm_object_create(c, &leaf) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&value, bad);
    if (!(fvm_object_set(c, inner, "first", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&value, good);
    if (!(fvm_object_set(c, outer, "first", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&value, leaf);
    if (!(fvm_object_set(c, good, "second", &value) == FVM_OK))
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
    object_value(&value, bad);
    if (!(fvm_object_set(c, bad, "child", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&value, good);
    if (!(fvm_object_set(c, good, "child", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, leaf, "victim", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&reference, "first/child:second.victim");
    fvm_value_string(&path, "first");
    fvm_value_string(&name, "second");
    fvm_value_undefined(&value);
    if (!(fvm_object_set(c, bad, "second", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present) == FVM_OK && present &&
          out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_delete_reference(c, scope, 0, 0, &reference, &out) == FVM_OK && !out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_has_own(c, leaf, "victim", &present) == FVM_OK && present))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_NULL;
    if (!(fvm_object_set(c, bad, "second", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present) == FVM_OK && present &&
          out.type == FVM_VALUE_NULL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, bad, "second", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present) == FVM_OK && present && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, bad, "second", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present) == FVM_OK && present &&
          out.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_delete_reference(c, scope, 0, 0, &reference, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &name, &present) == FVM_OK && name.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "missing");
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present) == FVM_OK && !present &&
          out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&path, 1);
    fvm_value_number(&out, 99);
    present = 99;
    if (!(fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present) == FVM_INVALID_ARGUMENT &&
          out.number == 99 && present == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
