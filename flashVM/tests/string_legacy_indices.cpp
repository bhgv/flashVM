#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include <cmath>

static unsigned primitive_calls, text_calls;


static FVM_Result primitive(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++primitive_calls;
    *out = object(self);
    return FVM_OK;
}


static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++text_calls;
    fvm_value_number(out, 2);
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
        FVM_Value value, args[2], out, klass;
        FVM_Object *str = 0, *arg = 0, *fn = 0;
        fvm_value_string(&value, "ABCDEFGHIJKLMNOP");
        if (!(fvm_construct(c, "String", 1, &value, &str) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const double numbers[] = {4294967297.9, -4294967295.9, 2147483648.0, HUGE_VAL};
        const char *expected[] = {"B", "B", "", "A"};
        for (unsigned i = 0; i < 4; ++i)
        {
            fvm_value_number(args, numbers[i]);
            if (!(fvm_object_call(c, str, "charAt", 1, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!std::strcmp(out.string, version ? expected[i] : "")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(args, "010");
        if (!(fvm_object_call(c, str, "charAt", 1, args, &out) == FVM_OK &&
              !std::strcmp(out.string, version >= 6 ? "I" : "K")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &arg) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, primitive, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = object(fn);
        if (!(fvm_object_set(c, arg, "valueOf", &value) == FVM_OK))
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
        if (!(fvm_object_set(c, arg, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        args[0] = object(arg);
        primitive_calls = text_calls = 0;
        if (!(fvm_object_call(c, str, "charAt", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::strcmp(out.string, version ? "A" : "C") && primitive_calls == 1 &&
              text_calls == (version ? 0U : 1U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 99);
        if (!(fvm_object_call(c, str, "charCodeAt", 1, args, &out) == FVM_OK &&
              (version == 5 ? out.number == 0 : out.number != out.number)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, -1);
        if (!(fvm_object_call(c, str, "charCodeAt", 1, args, &out) == FVM_OK && out.number != out.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, -1);
        if (!(fvm_object_call(c, str, "substr", 2, args, &out) == FVM_OK &&
              !std::strcmp(out.string, version ? "ABCDEFGHIJKLMNO" : "")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 4294967297.0);
        fvm_value_number(args + 1, 3);
        if (!(fvm_object_call(c, str, "substring", 2, args, &out) == FVM_OK &&
              !std::strcmp(out.string, version ? "BC" : "DEFGHIJKLMNOP")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *methods[] = {"slice", "substr", "substring"};
        for (unsigned i = 0; i < 3; ++i)
        {
            if (!(fvm_object_call(c, str, methods[i], 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_STRING))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            primitive_calls = text_calls = 0;
            if (!(borrow(c, str, methods[i], arg, 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(version ? out.type == FVM_VALUE_UNDEFINED && !text_calls
                          : out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "2") && text_calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!primitive_calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_get_global(c, "String", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "010");
        if (!(fvm_object_call(c, klass.object, "fromCharCode", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned short unit = 0;
        size_t count = 0;
        if (!(fvm_value_get_utf16(&out, &unit, 1, &count) == FVM_OK && count == 1 && unit == (version >= 6 ? 8 : 10)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->primitive_depth == 0 && c->string_depth == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
