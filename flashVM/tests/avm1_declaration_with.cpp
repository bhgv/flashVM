/* Loaded local declarations and named functions select With own slots before enclosing locals. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/execution.h"


static void block(Bytes &b, const Bytes &body)
{
    b.push_back(0x94);
    u16(b, 2);
    u16(b, (unsigned)body.size());
    for (size_t i = 0; i < body.size(); ++i)
    {
        b.push_back(body[i]);
    }
}


static void declare(Bytes &b, const char *key, bool initialized, unsigned value)
{
    text(b, key);
    if (initialized)
    {
        integer(b, value);
    }
    b.push_back(initialized ? 0x3c : 0x41);
}


static FVM_Value execute_function(FVM_Context *c, Bytes code)
{
    code.push_back(0);
    FVM_Object *fn = 0;
    FVM_Value out;
    if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}


static void definition(Bytes &b, const char *key)
{
    Bytes body;
    integer(body, 42);
    body.push_back(0x3e);
    body.push_back(0);
    Bytes header;
    name(header, key);
    u16(header, 0);
    u16(header, (unsigned)body.size());
    b.push_back(0x9b);
    u16(b, (unsigned)header.size());
    for (size_t i = 0; i < header.size(); ++i)
    {
        b.push_back(header[i]);
    }
    for (size_t i = 0; i < body.size(); ++i)
    {
        b.push_back(body[i]);
    }
}

static unsigned collected_functions;


static FVM_Result forbidden_getter(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *, void *)
{
    if (!(0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static FVM_Result collect_function(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->argc == 1 && frame->argv[0].type == FVM_VALUE_OBJECT && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value result;
    if (!(fvm_function_invoke(c, frame->argv[0].object, 0, 0, 0, &result) == FVM_OK && result.number == 42 &&
          fvm_set_global(c, "captured", frame->argv) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++collected_functions;
    fvm_value_undefined(out);
    return FVM_OK;
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
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *wrapped = 0, *proto = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &wrapped) == FVM_OK &&
              fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK &&
              fvm_object_set_prototype(c, wrapped, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, out;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = wrapped;
        if (!(fvm_set_global(c, "wrapped", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, wrapped, "own", &value) == FVM_OK &&
              fvm_object_set(c, proto, "inherited", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes own, code;
        declare(own, "own", true, 42);
        declare(code, "own", true, 31);
        variable(code, "wrapped");
        block(code, own);
        variable(code, "own");
        code.push_back(0x3e);
        out = execute_function(c, code);
        if (!(out.number == 31 && fvm_object_get(c, wrapped, "own", &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes inherited, code2;
        declare(inherited, "inherited", true, 42);
        variable(code2, "wrapped");
        block(code2, inherited);
        variable(code2, "inherited");
        code2.push_back(0x3e);
        out = execute_function(c, code2);
        if (!(out.number == 42 && fvm_object_get(c, proto, "inherited", &out) == FVM_OK && out.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes only, code3;
        declare(only, "missing", false, 0);
        declare(code3, "missing", true, 31);
        variable(code3, "wrapped");
        block(code3, only);
        variable(code3, "missing");
        code3.push_back(0x3e);
        if (!(execute_function(c, code3).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, wrapped, "named", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes functions, outer;
        definition(functions, "named");
        definition(functions, "inherited");
        variable(outer, "wrapped");
        block(outer, functions);
        outer.push_back(0);
        if (!(fvm_avm1_execute(c, outer.begin(), outer.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, wrapped, "named", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT &&
              fvm_function_invoke(c, value.object, 0, 0, 0, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "named", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
              fvm_get_global(c, "inherited", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT &&
              fvm_object_get(c, proto, "inherited", &out) == FVM_OK && out.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *getter = 0, *setter = 0;
        const unsigned char empty[] = {0};
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &getter) == FVM_OK &&
              fvm_function_set_body(c, getter, forbidden_getter, 0, 0) == FVM_OK &&
              fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &setter) == FVM_OK &&
              fvm_function_set_body(c, setter, collect_function, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value args[3];
        fvm_value_string(args, "collectedFn");
        fvm_value_undefined(args + 1);
        args[1].type = FVM_VALUE_OBJECT;
        args[1].object = getter;
        fvm_value_undefined(args + 2);
        args[2].type = FVM_VALUE_OBJECT;
        args[2].object = setter;
        if (!(fvm_object_call(c, wrapped, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes collecting, program;
        definition(collecting, "collectedFn");
        variable(program, "wrapped");
        block(program, collecting);
        program.push_back(0);
        if (!(fvm_avm1_execute(c, program.begin(), program.size()) == FVM_OK &&
              fvm_get_global(c, "captured", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT &&
              fvm_function_invoke(c, value.object, 0, 0, 0, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->lexical_scopes.empty() && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(collected_functions == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
