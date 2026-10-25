/* Native enumeration preserves dense name order, hiding, counted keys and versioned sentinels. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/movieclip.h"


static FVM_Result enum_forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static FVM_Object *enum_setup(FVM_Context *c)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *object = 0, *parent = 0;
    FVM_Value value;
    if (!(fvm_object_create(c, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &parent) == FVM_OK && fvm_object_set_prototype(c, object, parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, parent, "hidden", &value) == FVM_OK &&
          fvm_object_set(c, parent, "inherited", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, object, "hidden", &value) == FVM_OK &&
          fvm_object_set_property_flags(c, object, "hidden", FVM_PROPERTY_DONT_ENUM, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_bytes(c, object, "n\0k", 3, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, object, "accessor", enum_forbidden, enum_forbidden, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(object);
    if (!(fvm_set_global(c, "input", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return object;
}


static Program enum_program(bool named, unsigned count, const char *binding = "input")
{
    Program p;
    if (named)
    {
        p.text(binding);
    }
    else
    {
        p.variable(binding);
    }
    p.op(named ? 0x46 : 0x55);
    p.number(count + 1);
    p.op(0x42);
    p.finish();
    return p;
}


static void enum_result(FVM_Context *c, const FVM_Value &out, const FVM_Value &expected)
{
    if (!(out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t count = 0, size = 0;
    if (!(fvm_table_indexed_count(c, expected.object, &count) == FVM_OK &&
          fvm_table_indexed_count(c, out.object, &size) == FVM_OK && size == count + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < count; ++i)
    {
        FVM_Value a, b;
        if (!(fvm_table_get_index(c, out.object, (unsigned long)i, &a) == FVM_OK &&
              fvm_table_get_index(c, expected.object, (unsigned long)i, &b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(a.type == FVM_VALUE_STRING && b.type == FVM_VALUE_STRING &&
              fvm_value_string_size(&a) == fvm_value_string_size(&b)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(a.string, b.string, fvm_value_string_size(&a))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value end;
    if (!(fvm_table_get_index(c, out.object, (unsigned long)count, &end) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(end.type == (c->swf_loaded ? FVM_VALUE_UNDEFINED : FVM_VALUE_NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_ENUMERATE_FIXTURE_ONLY


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
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            if (version && version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Object *object = enum_setup(c);
            FVM_Value value = class_value(object), expected, out;
            if (!(fvm_exec_enumerate(c, &value, &expected) == FVM_OK && expected.object->indexed.size() == 3 &&
                  fvm_gc_pin(c, expected.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned named = 0; named < 2; ++named)
            {
                Program p = enum_program(named != 0, 3);
                out = class_run(c, p);
                enum_result(c, out, expected);
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                enum_result(c, out, expected);
            }
            /* A new property cannot mutate the previously returned name array. */
            fvm_value_number(&value, 42);
            if (!(fvm_object_set(c, object, "later", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            enum_result(c, out, expected);
            for (unsigned named = 0; named < 2; ++named)
            {
                Program p;
                p.number(17);
                p.op(named ? 0x46 : 0x55);
                p.finish();
                out = class_run(c, p, version ? FVM_OK : FVM_INVALID_ARGUMENT);
                if (version)
                {
                    if (!(out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                Program missing;
                missing.op(named ? 0x46 : 0x55);
                missing.finish();
                out = class_run(c, missing, version ? FVM_OK : FVM_ERROR);
                if (version)
                {
                    if (!(out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            FVM_Object *clip = 0;
            if (!(fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 17);
            if (!(fvm_object_set(c, clip, "visible", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(clip);
            if (!(fvm_set_global(c, "clip", &value) == FVM_OK && fvm_exec_enumerate(c, &value, &expected) == FVM_OK &&
                  fvm_gc_pin(c, expected.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!expected.object->indexed.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program clip_program = enum_program(true, (unsigned)expected.object->indexed.size(), "clip");
            out = class_run(c, clip_program, FVM_OK, clip);
            enum_result(c, out, expected);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
