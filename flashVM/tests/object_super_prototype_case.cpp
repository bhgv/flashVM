/* Legacy case aliases of super.__proto__ preserve ancestor projection and user overrides. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "object_super_accessors.cpp"
#include <cstring>

static unsigned prototype_reads;


static FVM_Result
prototype_marker(FVM_Context *c, FVM_Object *receiver, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++prototype_reads;
    return fvm_object_get(c, receiver, "marker", out);
}
#ifdef FVM_SUPER_PROTO_CASE_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#endif


static void prototype_read(FVM_Context *c, FVM_Object *facade, const char *key, size_t size, FVM_Value *out)
{
#ifdef FVM_SUPER_PROTO_CASE_JIT
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = facade;
    if (!(fvm_set_global(c, "prototypeFacade", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string_bytes(&value, key, size);
    if (!(fvm_set_global(c, "prototypeKey", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program read;
    read.variable("prototypeFacade");
    read.variable("prototypeKey");
    read.op(0x4e);
    read.finish();
    *out = class_run(c, read);
#else
    if (!(fvm_object_get_bytes(c, facade, key, size, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
}


static void prototype_defaults(FVM_Context *c, FVM_Object *facade, FVM_Object *expected, unsigned version)
{
    const char *names[] = {"__proto__", "__PROTO__", "__PrOtO__", "__PROTO__\0tail"};
    for (unsigned i = 0; i < 4; ++i)
    {
        FVM_Value out;
        prototype_read(c, facade, names[i], i == 3 ? 14 : std::strlen(names[i]), &out);
        if (i == 0 || (i < 3 && version <= 6))
        {
            if (!(out.type == FVM_VALUE_OBJECT && out.object == expected))
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
    }
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
#ifdef FVM_SUPER_PROTO_CASE_JIT
    if (!fvm_jit_available())
    {
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
        for (unsigned version = 5; version <= 8; ++version)
        {
            for (unsigned spelling = 0; spelling < 2; ++spelling)
            {
                FVM_Context *c = 0, *other = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                      fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
                      fvm_standard_install(other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *instance = 0, *target = 0, *facade = 0;
                super_accessor_setup(c, &instance, &target, &facade);
                FVM_Value klass, expected, base;
                if (!(fvm_get_global(c, "Object", &klass) == FVM_OK &&
                      fvm_object_get(c, klass.object, "prototype", &expected) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                prototype_defaults(c, facade, expected.object, version);
                FVM_Channel *channel = 0;
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = facade;
                if (!(fvm_channel_create(&channel) == FVM_OK &&
                      fvm_channel_send_value(c, channel, &value, 0) == FVM_OK &&
                      fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK &&
                      fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *copy_receiver = value.object->resource_result.object->properties["receiver"].object;
                prototype_defaults(other, value.object, copy_receiver->prototype->prototype->prototype, version);
                if (!(fvm_get_global(c, "Base", &base) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *override_name = spelling ? "__PROTO__" : "__proto__";
                if (!(fvm_object_define_accessor_rooted(
                          c, prototype(c, base), override_name, prototype_marker, 0, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *names[] = {"__proto__", "__PROTO__", "__PrOtO__", "__PROTO__\0tail"};
                prototype_reads = 0;
                unsigned wanted_reads = 0;
                for (unsigned i = 0; i < 4; ++i)
                {
                    prototype_read(c, facade, names[i], i == 3 ? 14 : std::strlen(names[i]), &value);
                    if (i < 3 && (version <= 6 || !std::strcmp(names[i], override_name)))
                    {
                        ++wanted_reads;
                        if (!(value.type == FVM_VALUE_NUMBER && value.number == 9))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (i == 0)
                    {
                        if (!(value.type == FVM_VALUE_OBJECT && value.object == expected.object))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(value.type == FVM_VALUE_UNDEFINED))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                if (!(prototype_reads == wanted_reads && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames &&
                      !other->gc_frames))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_channel_destroy(channel);
                fvm_context_destroy(other);
                fvm_context_destroy(c);
            }
        }
    fvm_runtime_destroy(rt);
}
