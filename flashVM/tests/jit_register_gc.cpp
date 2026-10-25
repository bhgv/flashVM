/* Objects retained only by native legacy/Function2/root register banks survive collecting getters. */
#define main jit_reference_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "jit_references.cpp"

#undef main
#include "../src/standard/function_state.hpp"

static FVM_Object *temporary;


static FVM_Result getter(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *collect)
{
    if (collect)
    {
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bool found = false;
        for (size_t i = 0; i < c->objects.size(); ++i)
        {
            if (c->objects[i] == temporary)
            {
                found = true;
            }
        }
        if (!(found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(out, 0);
        return FVM_OK;
    }
    if (!(fvm_object_create(c, &temporary) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, temporary, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = temporary;
    return FVM_OK;
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    int collect = 1;
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
        FVM_Object *owner = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, owner) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0};
        FVM_Value args[3], out;
        FVM_Object *setter = 0;
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &setter) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, fn, getter, i ? &collect : 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, i ? "collect" : "fresh");
            fvm_value_undefined(args + 1);
            args[1].type = FVM_VALUE_OBJECT;
            args[1].object = fn;
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_OBJECT;
            args[2].object = setter;
            if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        Program p;
        p.variable("owner");
        p.text("fresh");
        p.op(0x4e);
        p.op(0x87);
        p.op(1);
        p.op(0);
        p.op(2);
        p.op(0x17);
        p.variable("owner");
        p.text("collect");
        p.op(0x4e);
        p.op(0x17);
        p.op(0x96);
        p.op(2);
        p.op(0);
        p.op(4);
        p.op(2);
        p.text("alive");
        p.op(0x4e);
        p.finish();
        for (unsigned private_bank = 0; private_bank < 2; ++private_bank)
        {
            FVM_ScriptFunction definition;
            definition.body = p.bytes;
            definition.function2 = private_bank != 0;
            definition.register_count = 8;
            definition.parameters.push_back(fvm_detail::string("owner"));
            definition.parameter_registers.push_back(0);
            FVM_Object *fn = 0;
            FVM_JitCode *native = 0;
            if (!(fvm_function_make_script(c, definition, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &native) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(args);
            args[0].type = FVM_VALUE_OBJECT;
            args[0].object = owner;
            for (unsigned mode = 0; mode < 2; ++mode)
            {
                if (mode)
                {
                    if (!(fvm_function_set_body(c, fn, fvm_jit_body, native, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_GCRootFrame *roots = c->gc_frames;
                if (!(fvm_function_invoke(c, fn, 0, 1, args, &out) == FVM_OK && out.number == 42))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(c->gc_frames == roots && !native->active && !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(native) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_undefined(args);
        args[0].type = FVM_VALUE_OBJECT;
        args[0].object = owner;
        if (!(fvm_set_global(c, "owner", args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_ExecutionOptions options;
        if (!(fvm_execution_options_init(&options) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        options.mode = FVM_EXECUTION_JIT;
        if (!(fvm_avm1_execute_options(c, p.bytes.begin(), p.bytes.size(), &options) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
