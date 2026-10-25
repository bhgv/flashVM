/* Loaded AVM1 numeric object conversion uses valueOf only, with bounded reentry. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "../src/avm1/coercion.hpp"
#include "flashvm/gc.h"
#include "flashvm/property.h"

static unsigned strings, values;
static FVM_Value answer;


static FVM_Result stringify(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++strings;
    return fvm_value_string(out, "7");
}


static FVM_Result value_of(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++values;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = answer;
    return FVM_OK;
}


static FVM_Result recurse(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = self;
    double number = 0;
    FVM_Result r = fvm_avm1_number(c, &value, &number);
    fvm_value_number(out, number);
    return r;
}


static FVM_Value numeric(FVM_Context *c, FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    if (!(fvm_set_global(c, "input", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    global(b, "input");
    b.push_back(0x4a);
    return run(c, b);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 0, 16, 0, 0, 0, 0x08, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *object = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, object, "toString", stringify, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        strings = values = 0;
        FVM_Value result = numeric(c, object);
        if (!(version < 5 ? result.number == 0 : result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(strings == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value noncallable;
        fvm_value_number(&noncallable, 9);
        if (!(fvm_object_set(c, object, "valueOf", &noncallable) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        result = numeric(c, object);
        if (!(version < 7 ? result.number == 0 : result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(strings == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int deleted = 0;
        if (!(fvm_object_delete(c, object, "valueOf", &deleted) == FVM_OK && deleted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, object, "valueOf", value_of, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&answer, "010");
        values = 0;
        result = numeric(c, object);
        if (!(result.number == (version < 6 ? 10 : 8) && values == 1 && strings == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&answer);
        answer.type = FVM_VALUE_OBJECT;
        answer.object = object;
        values = 0;
        result = numeric(c, object);
        if (!(version < 5 ? result.number == 0 : result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(values == 1 && strings == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, object, "valueOf", recurse, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        if (!(fvm_set_global(c, "input", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes bad;
        global(bad, "input");
        bad.push_back(0x4a);
        bad.push_back(0);
        if (!(fvm_avm1_execute(c, bad.begin(), bad.size()) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, object, "valueOf", value_of, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&answer, 42);
        if (!(numeric(c, object).number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Missing valueOf on a raw global-like object is undefined, without a toString fallback. */
        FVM_Object *raw = 0;
        if (!(fvm_object_create(c, &raw) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        result = numeric(c, raw);
        if (!(version < 7 ? result.number == 0 : result.number != result.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
