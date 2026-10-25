/* Loaded apply captures incrementally under shared work, including large sparse sources. */
#define FVM_APPLY_LOADED_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_apply_loaded.cpp"
#include <cstdio>

static unsigned work_calls;
static bool inspect_large;


static FVM_Result work_target(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    ++work_calls;
    if (inspect_large)
    {
        if (!(n == 65537 && a[0].type == FVM_VALUE_UNDEFINED && a[n - 1].type == FVM_VALUE_NUMBER &&
              a[n - 1].number == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(out, n);
    return FVM_OK;
}


static FVM_Result work_apply(FVM_Context *c, FVM_Object *fn, FVM_Object *source, FVM_Value *out)
{
    FVM_Value args[2];
    fvm_value_undefined(args);
    args[0].type = FVM_VALUE_NULL;
    args[1] = apply_object(source);
    return fvm_object_call(c, fn, "apply", 2, args, out);
}


static FVM_Result work_length(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    FVM_Value text, number;
    fvm_value_string(&text, "123");
    FVM_Result status = fvm_call(c, "parseInt", 1, &text, &number);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!(number.number == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 2);
    return FVM_OK;
}


static FVM_Result large_script(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->argc == 65537 && frame->argv[65536].number == 77 && frame->arguments))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value length;
    if (!(fvm_object_get(c, frame->arguments, "length", &length) == FVM_OK && length.number == 65537))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->argv[0].type == FVM_VALUE_UNDEFINED && frame->argv[65536].number == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, frame->argc);
    return FVM_OK;
}
#ifndef FVM_APPLY_WORK_FIXTURE_ONLY


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
        FVM_Object *fn = 0, *source = 0;
        FVM_Value value, out;
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
        if (!(fvm_function_create_native(c, work_target, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 3);
        if (!(fvm_object_set(c, source, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        work_calls = 0;
        rt->config.instruction_limit = 2;
        if (!(work_apply(c, fn, source, &out) == FVM_ERROR && c->avm1_steps == 2 && !work_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 3;
        if (!(work_apply(c, fn, source, &out) == FVM_OK && out.number == 3 && c->avm1_steps == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        c->avm1_execution_depth = 1;
        c->avm1_steps = 7;
        rt->config.instruction_limit = 8;
        work_calls = 0;
        if (!(work_apply(c, fn, source, &out) == FVM_ERROR && c->avm1_steps == 8 && !work_calls &&
              c->avm1_execution_depth == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        c->avm1_steps = std::numeric_limits<unsigned long>::max();
        rt->config.instruction_limit = 0;
        if (!(work_apply(c, fn, source, &out) == FVM_ERROR &&
              c->avm1_steps == std::numeric_limits<unsigned long>::max() && c->avm1_execution_depth == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        c->avm1_execution_depth = 0;
        FVM_Object *conversion = 0, *body = 0;
        const unsigned char empty[] = {0};
        if (!(fvm_object_create(c, &conversion) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &body) == FVM_OK &&
              fvm_function_set_body(c, body, work_length, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = apply_object(body);
        if (!(fvm_object_set(c, conversion, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = apply_object(conversion);
        if (!(fvm_object_set(c, source, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        work_calls = 0;
        rt->config.instruction_limit = 4;
        if (!(work_apply(c, fn, source, &out) == FVM_ERROR && c->avm1_steps == 4 && !work_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 5;
        if (!(work_apply(c, fn, source, &out) == FVM_OK && out.number == 2 && c->avm1_steps == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 2147483647.0);
        if (!(fvm_object_set(c, source, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        work_calls = 0;
        rt->config.instruction_limit = 4;
        if (!(work_apply(c, fn, source, &out) == FVM_ERROR && c->avm1_steps == 4 && !work_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 65537);
        if (!(fvm_object_set(c, source, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 77);
        if (!(fvm_object_set(c, source, "65536", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 0;
        inspect_large = true;
        if (!(work_apply(c, fn, source, &out) == FVM_OK && out.number == 65537 && c->avm1_steps == 65537))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        inspect_large = false;
        FVM_Object *script_target = 0;
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &script_target) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, script_target) == FVM_OK &&
              fvm_function_set_body(c, script_target, large_script, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(work_apply(c, script_target, source, &out) == FVM_OK && out.number == 65537))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *many_names = 0;
        if (!(fvm_object_create(c, &many_names) == FVM_OK && fvm_gc_pin(c, many_names) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 25; ++i)
        {
            char name[16];
            std::sprintf(name, "field%u", i);
            fvm_value_number(&value, i);
            if (!(fvm_object_set(c, many_names, name, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        rt->config.instruction_limit = 8;
        work_calls = 0;
        FVM_Result status = work_apply(c, fn, many_names, &out);
        if (version < 7)
        {
            if (!(status == FVM_ERROR && c->avm1_steps == 8 && !work_calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(status == FVM_OK && out.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        rt->config.instruction_limit = 0;
        if (!(work_apply(c, fn, many_names, &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
