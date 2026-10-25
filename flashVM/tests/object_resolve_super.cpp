/* A missing super property resolves on the parent chain with original this; the input facade stays live. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "object_super_accessors.cpp"
#include "flashvm/property.h"
#include "flashvm/exception.h"

static FVM_Object *resolve_super_receiver, *resolve_super_facade;
static unsigned resolve_super_calls, resolve_super_throw;


static FVM_Result super_resolver(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++resolve_super_calls;
    if (!(frame->receiver == resolve_super_receiver && frame->argc == 1 && fvm_value_string_size(frame->argv) == 5 &&
          !std::memcmp(frame->argv[0].string, "X\0Key", 5)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(
              c, c->global->properties["Base"].object->properties["prototype"].object, "__resolve", &removed) ==
              FVM_OK &&
          removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, resolve_super_receiver, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == resolve_super_facade)
        {
            alive = true;
        }
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (resolve_super_throw)
    {
        fvm_value_number(out, 73);
        return fvm_exception_raise(c, out);
    }
    *out = frame->argv[0];
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (resolve_super_throw = 0; resolve_super_throw < 2; ++resolve_super_throw)
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
            FVM_Object *target = 0, *method = 0;
            super_accessor_setup(c, &resolve_super_receiver, &target, &resolve_super_facade);
            int removed = 0;
            if (!(fvm_object_delete(c, target, "slot", &removed) == FVM_OK && removed))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value base, value;
            if (!(fvm_get_global(c, "Base", &base) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned char end[] = {0};
            if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &method) == FVM_OK &&
                  fvm_function_set_body(c, method, super_resolver, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = method;
            if (!(fvm_object_set(c, prototype(c, base), "__resolve", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            resolve_super_calls = 0;
            if (!(fvm_object_get_bytes(c, resolve_super_facade, "X\0Key", 5, &value) ==
                      (resolve_super_throw ? FVM_EXCEPTION : FVM_OK) &&
                  resolve_super_calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (resolve_super_throw)
            {
                if (!(fvm_exception_get(c, &value) == FVM_OK && value.number == 73 && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 5 &&
                      !std::memcmp(value.string, "X\0Key", 5)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_gc_pin(c, resolve_super_facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
                  fvm_gc_unpin(c, resolve_super_facade) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->resolve_depth && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
