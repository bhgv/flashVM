#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include <cmath>

static unsigned calls;
static bool fail;


static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    if (fail)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 81);
    return FVM_OK;
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, args[2], out;
        FVM_Object *str = 0, *label = 0, *obj = 0, *fn = 0;
        fvm_value_string(&value, "ABAB");
        if (!(fvm_construct(c, "String", 1, &value, &str) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "[type Object]");
        if (!(fvm_construct(c, "String", 1, &value, &label) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &obj) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = object(fn);
        if (!(fvm_object_set(c, obj, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *methods[] = {"indexOf", "lastIndexOf"};
        for (unsigned backwards = 0; backwards < 2; ++backwards)
        {
            if (!(fvm_object_call(c, str, methods[backwards], 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_NUMBER && out.number == -1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(args);
            if (!(fvm_object_call(c, str, methods[backwards], 1, args, &out) == FVM_OK &&
                  out.number == (version && version < 7 ? backwards ? 4 : 0 : -1)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            args[0] = object(obj);
            calls = 0;
            fail = false;
            if (!(fvm_object_call(c, label, methods[backwards], 1, args, &out) == FVM_OK &&
                  out.number == (version ? 0 : -1) && calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            calls = 0;
            if (!(borrow(c, str, methods[backwards], obj, 0, 0, &out) == FVM_OK && calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_NUMBER && out.number == -1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail = true;
            calls = 0;
            if (!(borrow(c, str, methods[backwards], obj, 0, 0, &out) == FVM_ERROR && calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail = false;
        }
        fvm_value_string(args, "A");
        fvm_value_number(args + 1, -1);
        if (!(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == (version ? -1 : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args + 1, HUGE_VAL - HUGE_VAL);
        if (!(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == (version ? 0 : 2)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args + 1, HUGE_VAL);
        if (!(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == (version ? 0 : 2)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(args + 1);
        if (!(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args + 1, 4294967298.0);
        if (!(fvm_object_call(c, str, "indexOf", 2, args, &out) == FVM_OK && out.number == (version ? 2 : -1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "");
        fvm_value_number(args + 1, 99);
        if (!(fvm_object_call(c, str, "indexOf", 2, args, &out) == FVM_OK && out.number == (version ? -1 : 4)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == 0 && c->string_depth == 0 && c->primitive_depth == 0 && c->avm1_execution_depth == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
