/* Interpreted, attached and automatic native CallMethod share loaded apply preparation work. */
#define FVM_APPLY_WORK_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_apply_work.cpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        unsigned long baseline = 0;
        for (class_mode = 0; class_mode < 3; ++class_mode)
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
            if (!(fvm_function_create_native(c, work_target, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = apply_object(fn);
            if (!(fvm_set_global(c, "applyFunction", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_create(c, &source) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = apply_object(source);
            if (!(fvm_set_global(c, "applyList", &value) == FVM_OK))
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
            Program call;
            call.variable("applyList");
            call.op(0x96);
            call.op(1);
            call.op(0);
            call.op(2);
            call.number(2);
            call.variable("applyFunction");
            call.text("apply");
            call.op(0x52);
            call.finish();
            rt->config.instruction_limit = 0;
            out = class_run(c, call);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!class_mode)
            {
                baseline = c->avm1_steps;
            }
            else
            {
                if (!(c->avm1_steps == baseline))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(baseline > 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            work_calls = 0;
            rt->config.instruction_limit = baseline - 2;
            class_run(c, call, FVM_ERROR);
            if (!(!work_calls && c->avm1_steps == baseline - 2 && !c->avm1_execution_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            rt->config.instruction_limit = 0;
            out = class_run(c, call);
            if (!(out.number == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 2.9);
            if (!(fvm_object_set(c, source, "length", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            out = class_run(c, call);
            if (!(out.number == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 5);
            if (!(fvm_set_global(c, "applyList", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            out = class_run(c, call);
            if (!(out.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *plain = 0;
            if (!(fvm_object_create(c, &plain) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = apply_object(plain);
            if (!(fvm_set_global(c, "plainTarget", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = apply_object(source);
            if (!(fvm_set_global(c, "applyList", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, fn, "apply", &value) == FVM_OK &&
                  fvm_set_global(c, "borrowApply", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, fn, "call", &value) == FVM_OK && fvm_set_global(c, "borrowCall", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program borrowed_apply;
            borrowed_apply.variable("applyList");
            borrowed_apply.op(0x96);
            borrowed_apply.op(1);
            borrowed_apply.op(0);
            borrowed_apply.op(2);
            borrowed_apply.variable("plainTarget");
            borrowed_apply.number(3);
            borrowed_apply.variable("borrowApply");
            borrowed_apply.text("call");
            borrowed_apply.op(0x52);
            borrowed_apply.finish();
            work_calls = 0;
            out = class_run(c, borrowed_apply);
            if (!(out.type == FVM_VALUE_UNDEFINED && !work_calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program borrowed_call;
            borrowed_call.number(99);
            borrowed_call.op(0x96);
            borrowed_call.op(1);
            borrowed_call.op(0);
            borrowed_call.op(2);
            borrowed_call.variable("plainTarget");
            borrowed_call.number(3);
            borrowed_call.variable("borrowCall");
            borrowed_call.text("call");
            borrowed_call.op(0x52);
            borrowed_call.finish();
            out = class_run(c, borrowed_call);
            if (!(out.type == FVM_VALUE_UNDEFINED && !work_calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
