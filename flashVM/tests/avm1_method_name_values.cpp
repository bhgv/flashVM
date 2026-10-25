/* Loaded method-name coercion, callback roots, construction and bytecode bridge. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/exception.h"

static unsigned invoked, converted;
static bool fail_conversion;


static FVM_Result target(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(n == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++invoked;
    fvm_value_number(out, 42);
    return FVM_OK;
}


static FVM_Result convert(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *raw)
{
    ++converted;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail_conversion)
    {
        FVM_Value e;
        fvm_value_number(&e, 77);
        return fvm_exception_raise(c, &e);
    }
    return fvm_value_string(out, (const char *)raw);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
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
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *o = 0, *fn = 0, *key = 0, *converter = 0;
        FVM_Value owner, namev, value, out;
        if (!(fvm_object_create(c, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, target, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_object_set(c, o, "17", &value) == FVM_OK && fvm_object_set(c, o, "null", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, o, version == 4 ? "1" : "true", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_OBJECT;
        owner.object = o;
        fvm_value_number(&namev, 17);
        if (!(fvm_exec_call_method(c, 0, &owner, &namev, 0, 0, 0, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&namev);
        namev.type = FVM_VALUE_NULL;
        if (!(fvm_exec_call_method(c, 0, &owner, &namev, 0, 0, 0, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&namev);
        namev.type = FVM_VALUE_BOOLEAN;
        namev.boolean = 1;
        if (!(fvm_exec_call_method(c, 0, &owner, &namev, 0, 0, 0, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_set_global(c, "receiver", &owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes code;
        text(code, "result");
        integer(code, 0);
        variable(code, "receiver");
        integer(code, 17);
        code.push_back(0x52);
        code.push_back(0x1d);
        code.push_back(0);
        if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &key) == FVM_OK && fvm_gc_pin(c, key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char body[] = {0};
        if (!(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &converter) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, converter, convert, (void *)"charAt", 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = converter;
        if (!(fvm_object_set(c, key, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&owner, "abc");
        fvm_value_undefined(&namev);
        namev.type = FVM_VALUE_OBJECT;
        namev.object = key;
        fvm_value_number(&value, 1);
        if (!(fvm_exec_call_method(c, 0, &owner, &namev, 0, 1, &value, &out) == FVM_OK &&
              out.type == FVM_VALUE_STRING && out.string[0] == 'b'))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_conversion = true;
        fvm_value_number(&out, 99);
        if (!(fvm_exec_call_method(c, 0, &owner, &namev, 0, 1, &value, &out) == FVM_EXCEPTION && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 77 && fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_conversion = false;
        unsigned before = converted;
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_NULL;
        if (!(fvm_exec_call_method(c, 1, &owner, &namev, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
              converted == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        owner.type = FVM_VALUE_OBJECT;
        owner.object = o;
        fvm_value_number(&value, 3);
        if (!(fvm_object_set(c, o, "plain", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&namev, "plain");
        if (!(fvm_exec_call_method(c, 1, &owner, &namev, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(invoked == 20 && converted == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
}
