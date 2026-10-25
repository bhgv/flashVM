/* Imported inherited AVM1 tick handlers survive sender teardown and collection. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "movieclip_tick_fixture.hpp"
#include "flashvm/serialization.h"


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
        FVM_Context *sender = tick_context(rt, version), *dest = tick_context(rt, version);
        FVM_Object *root = tick_root(sender), *proto = 0, *fn = 0;
        if (!(fvm_object_create(sender, &proto) == FVM_OK && fvm_object_set_prototype(sender, root, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program p;
        p.variable("this");
        p.text("seen");
        p.number(42);
        p.op(0x4f);
        p.op(0);
        if (!(fvm_function_create_avm1(sender, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value = tick_object(fn);
        if (!(fvm_object_set(sender, proto, "onEnterFrame", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = tick_object(root);
        unsigned char *wire = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(sender, &value, &wire, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(sender);
        if (!(fvm_value_deserialize(dest, wire, size, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(wire);
        root = value.object;
        if (!(fvm_gc_pin(dest, root) == FVM_OK && fvm_gc_string_mode(dest, FVM_GC_STRINGS_TRACED) == FVM_OK &&
              fvm_gc_collect(dest, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(dest, root, 2) == FVM_OK && fvm_object_get(dest, root, "seen", &value) == FVM_OK &&
              value.type == FVM_VALUE_NUMBER && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(dest);
        fvm_context_destroy(dest);
    }
    fvm_runtime_destroy(rt);
}
