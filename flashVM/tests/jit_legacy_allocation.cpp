/* Allocation failures in native legacy/logical operations, coercions and left/right hooks restore roots. */
#define main legacy_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_LEGACY_CALLBACK_FIXTURE_ONLY
#include "jit_legacy_callbacks.cpp"


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
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (unsigned kind = 0; kind < 4; ++kind)
        {
            FVM_Context *c = 0;
            const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (loaded)
            {
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
            FVM_Value args[2];
            unsigned ids[] = {1, 2};
            if (kind == 1)
            {
                args[0] = legacy_object(c, ids);
                args[1] = legacy_object(c, ids + 1);
            }
            else if (kind == 0)
            {
                fvm_value_string(args, "0");
                fvm_value_string(args + 1, "1");
            }
            else
            {
                fvm_value_number(args, 1);
                fvm_value_number(args + 1, 2);
                FVM_Object *meta = 0, *hook = 0;
                if (!(fvm_object_create(c, &meta) == FVM_OK && fvm_gc_pin(c, meta) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char empty[] = {0};
                if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &hook) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_set_body(c, hook, legacy_hook, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = hook;
                if (!(fvm_object_set(c, meta, "__eq", &value) == FVM_OK &&
                      fvm_object_set(c, meta, "__lt", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned side = kind == 3 ? 1 : 0;
                if (!(fvm_value_set_metatable(c, args + side, meta) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                expected_left = 1;
                expected_right = 2;
                expected_owner = side ? 2 : 1;
            }
            Program p;
            const unsigned char operations[] = {0x0e, 0x0f, 0x10, 0x11};
            for (unsigned i = 0; i < 4; ++i)
            {
                p.variable("a");
                p.variable("b");
                p.op(operations[i]);
                if (i < 3)
                {
                    p.op(0x17);
                }
            }
            p.finish();
            const char *params[] = {"a", "b"};
            FVM_Object *fn = 0;
            FVM_JitCode *code = 0;
            FVM_Value out;
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 2, params, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 3000 && !success; ++boundary)
            {
                FVM_GCRootFrame *roots = c->gc_frames;
                legacy_count = 0;
                meta_calls = 0;
                allowance = boundary;
                FVM_Result status = fvm_function_invoke(c, fn, 0, 2, args, &out);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->script_depth && !c->gc_native_depth && !code->active &&
                      !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OK)
                {
                    success = true;
                    if (!(out.type == FVM_VALUE_BOOLEAN && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OUT_OF_MEMORY))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(success && failures && fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK &&
                  fvm_jit_destroy(code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
