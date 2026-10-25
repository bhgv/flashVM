#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include <string.h>


static FVM_Result plain(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o, void *u)
{
    (void)c;
    (void)s;
    (void)n;
    (void)a;
    (void)u;
    return fvm_value_string(o, "cabca");
}


static FVM_Result throws(FVM_Context *c, FVM_Object *s, unsigned int n, const FVM_Value *a, FVM_Value *o, void *u)
{
    FVM_Value error;
    (void)s;
    (void)n;
    (void)a;
    (void)o;
    (void)u;
    fvm_value_string(&error, "string conversion failed");
    return fvm_exception_raise(c, &error);
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *s = 0, *ordinary = 0;
    FVM_Value input, args[2], out, klass, prototype;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&input, "ababa");
    if (!(fvm_construct(c, "String", 1, &input, &s) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(args, "ba");
    if (!(fvm_object_call(c, s, "indexOf", 1, args, &out) == FVM_OK && out.number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, s, "lastIndexOf", 1, args, &out) == FVM_OK && out.number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 2);
    if (!(fvm_object_call(c, s, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 3);
    if (!(fvm_object_call(c, s, "indexOf", 2, args, &out) == FVM_OK && out.number == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 5);
    if (!(fvm_object_call(c, s, "indexOf", 2, args, &out) == FVM_OK && out.number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, -0.5);
    if (!(fvm_object_call(c, s, "charAt", 1, args, &out) == FVM_OK && !strcmp(out.string, "a")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, -999);
    fvm_value_number(args + 1, 999);
    if (!(fvm_object_call(c, s, "substring", 2, args, &out) == FVM_OK && !strcmp(out.string, "ababa")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 4);
    fvm_value_number(args + 1, 2);
    if (!(fvm_object_call(c, s, "slice", 2, args, &out) == FVM_OK && !strcmp(out.string, "")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 2);
    fvm_value_number(args + 1, -1);
    if (!(fvm_object_call(c, s, "substr", 2, args, &out) == FVM_OK && !strcmp(out.string, "")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "String", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 65536 + 65);
    if (!(fvm_object_call(c, klass.object, "fromCharCode", 1, args, &out) == FVM_OK && !strcmp(out.string, "A")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, -1);
    if (!(fvm_object_call(c, klass.object, "fromCharCode", 1, args, &out) == FVM_OK &&
          !strcmp(out.string, "\357\277\277")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, klass.object, "fromCharCode", 0, 0, &out) == FVM_OK && !strcmp(out.string, "")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&input);
    if (!(fvm_construct(c, "String", 1, &input, &s) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, s, "valueOf", 0, 0, &out) == FVM_OK && !strcmp(out.string, "undefined")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, klass.object, "prototype", &prototype) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &ordinary) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, ordinary, prototype.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, ordinary, "toString", plain, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 1);
    if (!(fvm_object_call(c, ordinary, "charAt", 1, args, &out) == FVM_OK && !strcmp(out.string, "a")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, ordinary, "valueOf", 0, 0, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, ordinary, "toString", throws, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    input.type = FVM_VALUE_OBJECT;
    input.object = ordinary;
    if (!(fvm_construct(c, "String", 1, &input, &s) == FVM_EXCEPTION && !s))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exception_get(c, &out) == FVM_OK && !strcmp(out.string, "string conversion failed")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&out, 7);
    if (!(fvm_object_set(c, klass.object, "prototype", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "String", 0, 0, &s) == FVM_INVALID_ARGUMENT && !s))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, klass.object, "prototype", &prototype) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, klass.object, "fromCharCode", plain, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
