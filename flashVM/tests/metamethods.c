#include <stdio.h>
#include <stdlib.h>
#include "flashvm/flashvm.h"
#include "flashvm/value.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include <string.h>

static int gc_count = 0;


static FVM_Result on_gc(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a, FVM_Value *out, void *u)
{
    (void)c;
    (void)u;
    if (!(n == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[0].object == self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[2].object == self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++gc_count;
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result
on_index(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a, FVM_Value *out, void *u)
{
    (void)c;
    (void)u;
    if (!(n == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[0].object == self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(strcmp(a[2].string, "missing") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 77);
    return FVM_OK;
}


static FVM_Result on_iter(FVM_Context *c, FVM_Object *self, unsigned int n, const FVM_Value *a, FVM_Value *out, void *u)
{
    (void)c;
    (void)u;
    if (!(n == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[0].object == self))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (a[2].number == 0)
    {
        fvm_value_number(out, 12);
    }
    else
    {
        fvm_value_undefined(out);
    }
    return FVM_OK;
}


int main(void)
{
    FVM_Config config;
    FVM_Runtime *r = 0;
    FVM_Context *c = 0;
    FVM_Object *o = 0, *m = 0;
    FVM_Value v, k;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(r, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &m) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, m, "__gc", on_gc, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, m, "__index", on_index, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_register(c, m, "__iterate", on_iter, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_table_set_metatable(c, o, m) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, o, "missing", &v) == FVM_OK && v.number == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_iterate(c, o, 0, &k, &v) == FVM_OK && v.number == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_iterate(c, o, 1, &k, &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_finalize(c, o) == FVM_OK && gc_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_meta_finalize(c, o) == FVM_OK && gc_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    if (!(gc_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
