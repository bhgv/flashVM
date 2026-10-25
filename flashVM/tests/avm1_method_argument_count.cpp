/* Call counts are values: conversion, wrapping, shortage and callback failure. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include <limits>

static unsigned received;


static FVM_Result collect_args(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{
    received = n;
    if (n)
    {
        if (!(n == 1 && args[0].number == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(out, n);
    return FVM_OK;
}


static FVM_Result construct_body(FVM_Context *, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    received = frame->argc;
    if (!(received == 1 && frame->argv[0].number == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, received);
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
        FVM_Object *o = 0, *fn = 0, *count_object = 0;
        FVM_Value value, result;
        unsigned count = 99;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "1");
        if (!(fvm_exec_argument_count(c, &value, 1, &count) == FVM_ERROR && count == 99))
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
        const double inputs[] = {
            1.9, -1, 4294967297.0, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()};
        const unsigned expected[] = {1, 3, 1, 0, 0};
        for (unsigned i = 0; i < 5; ++i)
        {
            fvm_value_number(&value, inputs[i]);
            if (!(fvm_exec_argument_count(c, &value, 3, &count) == FVM_OK && count == expected[i]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(&value, "2");
        if (!(fvm_exec_argument_count(c, &value, 1, &count) == FVM_OK && count == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, -1);
        count = 99;
        if (!(fvm_exec_argument_count(c, &value, FVM_EXEC_MAX_METHOD_ARGUMENTS + 1U, &count) == FVM_ERROR &&
              count == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, collect_args, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_object_set(c, o, "m", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = o;
        if (!(fvm_set_global(c, "receiver", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes body;
        integer(body, 77);
        variable(body, "count");
        variable(body, "receiver");
        text(body, "m");
        body.push_back(0x52);
        body.push_back(0x3e);
        body.push_back(0);
        if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK &&
              fvm_gc_pin(c, fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "9");
        if (!(fvm_set_global(c, "count", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, fn, 0, 0, 0, &result) == FVM_OK && result.number == 1 && received == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 0.9);
        if (!(fvm_set_global(c, "count", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, fn, 0, 0, 0, &result) == FVM_OK && result.number == 0 && received == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        attach(c, o, "m", construct_body, 0);
        body[body.size() - 3] = 0x53;
        if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "9");
        if (!(fvm_set_global(c, "count", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, fn, 0, 0, 0, &result) == FVM_OK && result.type == FVM_VALUE_OBJECT &&
              received == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &count_object) == FVM_OK && fvm_gc_pin(c, count_object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned id = 2;
        attach(c, count_object, "valueOf", numeric, &id);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = count_object;
        calls = 0;
        if (!(fvm_exec_argument_count(c, &value, 1, &count) == FVM_OK && count == 1 && calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failing = true;
        calls = 0;
        count = 99;
        if (!(fvm_exec_argument_count(c, &value, 1, &count) == FVM_EXCEPTION && count == 99 && calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failing = false;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
