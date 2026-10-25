/* Native member writes retain script setter captures/arguments through GC and thrown values. */
#define main jit_reference_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "jit_references.cpp"

#undef main
#define FVM_SETTER_COLLECTOR_FIXTURE_ONLY
#include "script_setter_collector.c"


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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        FVM_Object *owner = 0, *plain = 0, *capture = 0, *callback = 0, *input = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        const unsigned char empty[] = {0};
        FVM_Value value, args[2], out;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_gc_pin(c, owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK && fvm_object_create(c, &capture) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 99);
        if (!(fvm_object_set(c, capture, "alive", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &callback) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, callback, setter, 0, capture) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 11);
        if (!(fvm_object_set(c, owner, "value", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        install(c, owner, plain, callback);
        if (!(fvm_object_create(c, &input) == FVM_OK && fvm_gc_pin(c, input) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 42);
        if (!(fvm_object_set(c, input, "alive", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned qualified = 0; qualified < 2; ++qualified)
        {
            if (qualified && !version)
            {
                continue;
            }
            Program p;
            if (qualified)
            {
                p.text("input.value");
            }
            else
            {
                p.variable("input");
                p.text("value");
            }
            p.variable("assigned");
            p.op(qualified ? 0x1d : 0x4f);
            p.variable("input");
            p.text("value");
            p.op(0x4e);
            p.finish();
            FVM_Object *function = 0;
            FVM_JitCode *native = 0;
            const char *parameters[] = {"input", "assigned"};
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 2, parameters, 0, &function) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, function) == FVM_OK &&
                  fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &native) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(args);
            args[0].type = FVM_VALUE_OBJECT;
            args[0].object = owner;
            fvm_value_undefined(args + 1);
            args[1].type = FVM_VALUE_OBJECT;
            args[1].object = input;
            for (unsigned native_mode = 0; native_mode < 2; ++native_mode)
            {
                if (native_mode)
                {
                    if (!(fvm_function_set_body(c, function, fvm_jit_body, native, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                for (unsigned throwing = 0; throwing < 2; ++throwing)
                {
                    mode = throwing ? 2 : 0;
                    setter_entries = 0;
                    FVM_GCRootFrame *roots = c->gc_frames;
                    FVM_Result status = fvm_function_invoke(c, function, 0, 2, args, &out);
                    if (!(setter_entries == 1 && c->gc_frames == roots && !c->gc_native_depth && !c->script_depth &&
                          !c->avm1_execution_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (throwing)
                    {
                        if (!(status == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(status == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object == input))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            if (!(fvm_function_set_body(c, function, 0, 0, 0) == FVM_OK && fvm_jit_destroy(native) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
