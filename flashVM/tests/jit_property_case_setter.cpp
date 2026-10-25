/* Native SetMember preserves folded inherited setter dispatch and collecting failures. */
#define main property_case_setter_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "object_property_case_setter.cpp"

#undef main
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
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            for (setter_fault = 0; setter_fault < 4; ++setter_fault)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
                      fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const bool folded = version <= 6;
                FVM_Object *receiver = 0, *parent = 0, *get = 0, *set = 0;
                FVM_Value args[3], out, value;
                if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                      fvm_object_set_prototype(c, receiver, parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char end[] = {0};
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK &&
                      fvm_function_set_body(c, get, case_get, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK &&
                      fvm_function_set_body(c, set, case_set, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string_bytes(args, "MiXeD\0Name", 10);
                args[1] = setter_object(get);
                args[2] = setter_object(set);
                if (!(fvm_object_call(c, parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                expected_receiver = receiver;
                expected_owner = parent;
                setter_calls = 0;
                fvm_value_string_bytes(&value, "a\0b", 3);

                FVM_Value global = setter_object(receiver);
                if (!(fvm_set_global(c, "caseReceiver", &global) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string_bytes(&global, "mixed\0name", 10);
                if (!(fvm_set_global(c, "caseKey", &global) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_set_global(c, "caseValue", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program p;
                p.variable("caseReceiver");
                p.variable("caseKey");
                p.variable("caseValue");
                p.op(0x4f);
                p.number(99);
                p.finish();
                FVM_Result expected = folded && setter_fault == 1   ? FVM_ERROR
                                      : folded && setter_fault == 2 ? FVM_EXCEPTION
                                                                    : FVM_OK;
                out = class_run(c, p, expected);
                if (!(setter_calls == (folded ? 1U : 0U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (expected == FVM_OK)
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 99))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->calling_setter_owner && !c->calling_setter_name && !c->gc_frames && !c->gc_native_depth &&
                      !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
