/* Native scene bridges retain ordered collecting coercions and skip readonly/invalid writes. */
#define FVM_DISPLAY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_display.cpp"

static unsigned sequence[8], conversion_count, fail_conversion;
static double property_index;


static FVM_Result display_convert(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    if (!(conversion_count < 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    sequence[conversion_count++] = id;
    FVM_Result status = fvm_gc_collect(c, 0);
    if (status != FVM_OK)
    {
        return status;
    }
    if (id == 2)
    {
        fvm_value_string(out, "child");
    }
    else
    {
        fvm_value_number(out, id == 1 ? property_index : 17);
    }
    return fail_conversion == id ? fvm_exception_raise(c, out) : FVM_OK;
}


static void display_coercion(FVM_Context *c, const char *name, unsigned *id)
{
    const unsigned char empty[] = {0};
    FVM_Object *object = 0, *fn = 0;
    FVM_Value value;
    if (!(fvm_object_create(c, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, display_convert, id, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(fn);
    if (!(fvm_object_set(c, object, *id == 2 ? "toString" : "valueOf", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(object);
    if (!(fvm_set_global(c, name, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_DISPLAY_CALLBACK_FIXTURE_ONLY


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
            FVM_Object *child = 0, *root = display_setup(c, &child);
            unsigned ids[] = {1, 2, 3};
            display_coercion(c, "Selector", ids);
            display_coercion(c, "Path", ids + 1);
            display_coercion(c, "Scalar", ids + 2);
            Program p;
            p.variable("Path");
            p.variable("Selector");
            p.variable("Scalar");
            p.op(0x23);
            p.number(42);
            p.finish();
            property_index = 0;
            for (fail_conversion = 0; fail_conversion < 4; ++fail_conversion)
            {
                conversion_count = 0;
                FVM_Value out = class_run(c, p, fail_conversion ? FVM_EXCEPTION : FVM_OK, root);
                if (!(conversion_count == (fail_conversion ? fail_conversion : 3U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned i = 0; i < conversion_count; ++i)
                {
                    if (!(sequence[i] == i + 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!fail_conversion)
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            property_index = 20;
            fail_conversion = 3;
            conversion_count = 0;
            class_run(c, p, FVM_OK, root);
            if (!(conversion_count == 2 && sequence[0] == 1 && sequence[1] == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            property_index = 99;
            conversion_count = 0;
            class_run(c, p, FVM_OK, root);
            if (!(conversion_count == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program missing;
            missing.text("missing");
            missing.number(0);
            missing.variable("Scalar");
            missing.op(0x23);
            missing.number(42);
            missing.finish();
            conversion_count = 0;
            class_run(c, missing, FVM_OK, root);
            if (!(!conversion_count))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail_conversion = 0;
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
