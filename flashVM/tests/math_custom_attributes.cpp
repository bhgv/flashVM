/* First-load Math activation preserves custom host state and later script changes. */
#include <stdio.h>
#include <stdlib.h>
#include "math_attribute_fixture.hpp"

static unsigned math_custom_reads;


static FVM_Result math_custom_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++math_custom_reads;
    fvm_value_number(out, 64);
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
    const char *names[] = {"PI", "abs"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned method = 0; method < 2; ++method)
        {
            for (unsigned kind = 0; kind < 8; ++kind)
            {
                if (kind == 7 && method)
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value math, value;
                if (!(fvm_get_global(c, "Math", &math) == FVM_OK && fvm_gc_pin(c, math.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned expected = method ? 1U : 0U;
                if (kind == 1)
                {
                    expected = method ? 0U : 1U;
                    if (!(fvm_object_set_property_flags(c, math.object, names[method], expected, method ? 7 : 0) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 2)
                {
                    fvm_value_number(&value, 81);
                    if (!(fvm_object_set(c, math.object, names[method], &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 3)
                {
                    int removed = 0;
                    if (!(fvm_object_delete(c, math.object, names[method], &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_define_accessor_rooted(
                              c, math.object, names[method], math_custom_getter, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 4)
                {
                    int removed = 0;
                    if (!(fvm_object_delete(c, math.object, names[method], &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 5)
                {
                    FVM_Object *replacement = 0;
                    if (!(fvm_object_create(c, &replacement) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(replacement);
                    if (!(fvm_set_global(c, "Math", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 6)
                {
                    int removed = 0;
                    if (!(fvm_object_delete(c, c->global, "Math", &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_define_accessor_rooted(
                              c, c->global, "Math", math_custom_getter, 0, 0, math.object) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 7)
                {
                    FVM_Object *meta = 0;
                    if (!(fvm_construct(c, "Object", 0, 0, &meta) == FVM_OK &&
                          fvm_object_get(c, math.object, names[method], &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value.metatable = meta;
                    if (!(fvm_object_set(c, math.object, names[method], &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind != 4)
                {
                    if (!(fvm_object_property_flags(c, math.object, names[method], &expected) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                math_custom_reads = 0;
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && !math_custom_reads))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!kind)
                {
                    expected = 7;
                }
                if (kind == 4)
                {
                    int own = 1;
                    if (!(fvm_object_has_own(c, math.object, names[method], &own) == FVM_OK && !own))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    math_flag(c, math.object, names[method], expected);
                }
                if (!kind)
                {
                    FVM_Value args[4], out;
                    args[0] = math;
                    fvm_value_string(args + 1, method ? "ABS" : "pi");
                    fvm_value_number(args + 2, method ? 1 : 0);
                    fvm_value_number(args + 3, 7);
                    if (!(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    expected = method ? 1U : 0U;
                    math_flag(c, math.object, names[method], expected);
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    math_flag(c, math.object, names[method], expected);
                    fvm_value_number(&value, 81);
                    if (!(fvm_object_set(c, math.object, names[method], &value) == FVM_OK &&
                          fvm_object_get(c, math.object, names[method], &out) == FVM_OK && out.number == 81))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!math_custom_reads && !c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
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
