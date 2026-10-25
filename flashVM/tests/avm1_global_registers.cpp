/* Shared loaded register values survive activations and GC; private Function2 banks stay isolated. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"


struct Probe
{
    bool loaded;
};


static FVM_Result native_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Value value;
    fvm_value_number(&value, 99);
    if (!(fvm_exec_global_register(c, 1, 1, &value, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->register_count > 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (((Probe *)raw)->loaded)
    {
        if (!(frame->registers[1].type == FVM_VALUE_NUMBER && frame->registers[1].number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(frame->registers[1].type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return FVM_OK;
}


static bool present(FVM_Context *c, FVM_Object *object)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == object)
        {
            return true;
        }
    }
    return false;
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
    for (unsigned version = 4; version <= 9; ++version)
    {
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
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
        Bytes store;
        integer(store, 42);
        store.push_back(0x87);
        u16(store, 1);
        store.push_back(1);
        store.push_back(0x17);
        store.push_back(0);
        if (!(fvm_avm1_execute(c, store.begin(), store.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes read;
        reg(read, 1);
        end(read);
        FVM_Object *reader = 0;
        if (!(fvm_function_create_avm1(c, read.begin(), read.size(), 0, 0, 0, &reader) == FVM_OK &&
              fvm_gc_pin(c, reader) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value out;
        if (!(fvm_function_invoke(c, reader, 0, 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        Probe probe = {version <= 8};
        if (!(fvm_function_set_body(c, reader, native_body, &probe, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, reader, 0, 0, 0, &out) == FVM_OK && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, reader, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            Bytes local;
            integer(local, 88);
            local.push_back(0x87);
            u16(local, 1);
            local.push_back(1);
            local.push_back(0x17);
            reg(local, 1);
            end(local);
            FVM_Object *fn = define(c, 2, 0, 0, 0, 0, local);
            if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK && out.number == 88))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fn = define(c, 0, 0, 0, 0, 0, read);
            if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK && out.number == 99))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *object = 0, *meta = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        if (!(fvm_exec_global_register(c, 1, 0, &value, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        char bytes[] = {'a', 0, 'b'};
        fvm_value_string_bytes(&value, bytes, sizeof(bytes));
        value.metatable = meta;
        if (!(fvm_exec_global_register(c, 1, 2, &value, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bytes[0] = 'x';
        bytes[2] = 'y';
        if (!(fvm_gc_collect(c, 0) == FVM_OK && present(c, object) && present(c, meta)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_global_register(c, 0, 2, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
              out.string_size == 3 && !std::memcmp(out.string, "a\0b", 3) && out.metatable == meta))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_global_register(other, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(fvm_exec_global_register(c, 1, 0, &value, &out) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
              !present(c, object)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_global_register(c, 0, FVM_EXEC_GLOBAL_REGISTER_COUNT, 0, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->script_depth && !c->avm1_target_active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
