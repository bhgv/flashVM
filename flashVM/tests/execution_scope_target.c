/* C89 complete-path retries, retained With scopes and selected-root fallback. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"


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
    FVM_Object *inner, *outer, *bad, *good, *leaf, *scope, *target;
    FVM_Value value, path, out;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &inner) == FVM_OK && fvm_object_create(c, &outer) == FVM_OK &&
          fvm_object_create(c, &bad) == FVM_OK && fvm_object_create(c, &good) == FVM_OK &&
          fvm_object_create(c, &leaf) == FVM_OK && fvm_object_create(c, &target) == FVM_OK))
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
    fvm_value_string(&path, "first/second");
    if (!(fvm_exec_scope_target(c, scope, target, 0, &path, &out) == FVM_OK && out.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_scope_target(c, scope, target, 0, &path, &path) == FVM_OK && path.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&path, "missing/second");
    if (!(fvm_exec_scope_target(c, scope, target, 0, &path, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    object_value(&value, leaf);
    if (!(fvm_object_set(c, target, "fallback", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&path, "/fallback");
    if (!(fvm_exec_scope_target(c, scope, target, 0, &path, &out) == FVM_OK && out.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&path, "_root.fallback");
    if (!(fvm_exec_scope_target(c, scope, target, 0, &path, &out) == FVM_OK && out.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&path, "this/second");
    if (!(fvm_exec_scope_target(c, scope, target, good, &path, &out) == FVM_OK && out.object == leaf))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&path, 1);
    fvm_value_number(&out, 99);
    if (!(fvm_exec_scope_target(c, scope, target, 0, &path, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
