/* Native inheritance/interfaces and membership match interpretation in isolated versioned contexts. */
#define main class_reference_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "jit_references.cpp"

#undef main
#include "flashvm/exception.h"

static unsigned class_mode;


static FVM_Value class_value(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}


static FVM_Object *class_create(FVM_Context *c, const char *name)
{
    const unsigned char empty[] = {0};
    FVM_Object *object = 0;
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &object) == FVM_OK &&
          fvm_gc_pin(c, object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value = class_value(object);
    if (!(fvm_set_global(c, name, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return object;
}


static FVM_Value class_run(FVM_Context *c, const Program &p, FVM_Result expected = FVM_OK, FVM_Object *receiver = 0)
{
    FVM_Object *fn = 0;
    FVM_JitCode *code = 0;
    FVM_Value out;
    fvm_value_undefined(&out);
    if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
          fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode == 1)
    {
        if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    unsigned long entries = c->execution_mode.stats.native_entries;
    FVM_GCRootFrame *roots = c->gc_frames;
    if (!(fvm_function_invoke(c, fn, receiver, 0, 0, &out) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->execution_mode.stats.native_entries == entries + (class_mode != 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && !code->active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected == FVM_EXCEPTION)
    {
        if (!(fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (expected == FVM_OK && out.type == FVM_VALUE_OBJECT)
    {
        if (!(fvm_gc_pin(c, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}
#ifndef FVM_CLASS_FIXTURE_ONLY


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
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *base = class_create(c, "Base"), *child = class_create(c, "Child"),
                       *iface = class_create(c, "Iface"), *other = class_create(c, "Other");
            FVM_Value proto, value;
            if (!(fvm_object_get(c, base, "prototype", &proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 42);
            if (!(fvm_object_set(c, proto.object, "marker", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program extend;
            extend.variable("Child");
            extend.variable("Base");
            extend.op(0x69);
            extend.number(42);
            extend.finish();
            value = class_run(c, extend);
            if (!(value.type == FVM_VALUE_NUMBER && value.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *item = 0;
            if (!(fvm_function_construct(c, child, 0, 0, &item) == FVM_OK && fvm_gc_pin(c, item) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(item);
            if (!(fvm_set_global(c, "item", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, item, "marker", &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
                  value.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program instance;
            instance.variable("item");
            instance.variable("Base");
            instance.op(0x54);
            instance.finish();
            value = class_run(c, instance);
            if (!(value.type == FVM_VALUE_BOOLEAN && value.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program cast;
            cast.variable("Base");
            cast.variable("item");
            cast.op(0x2b);
            cast.finish();
            value = class_run(c, cast);
            if (!(value.type == FVM_VALUE_OBJECT && value.object == item))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program failed;
            failed.variable("Other");
            failed.variable("item");
            failed.op(0x2b);
            failed.finish();
            value = class_run(c, failed);
            if (!(value.type == FVM_VALUE_NULL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program interfaces;
            interfaces.variable("Iface");
            interfaces.number(1);
            interfaces.variable("Child");
            interfaces.op(0x2c);
            interfaces.number(42);
            interfaces.finish();
            value = class_run(c, interfaces);
            if (!(value.type == FVM_VALUE_NUMBER && value.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program membership;
            membership.variable("item");
            membership.variable("Iface");
            membership.op(0x54);
            membership.finish();
            value = class_run(c, membership);
            if (!(value.type == FVM_VALUE_BOOLEAN && !!value.boolean == (!version || version >= 7)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version >= 7)
            {
                FVM_Value old;
                FVM_Object *replacement = 0, *alias = class_create(c, "Alias");
                if (!(fvm_object_get(c, iface, "prototype", &old) == FVM_OK &&
                      fvm_object_create(c, &replacement) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(replacement);
                if (!(fvm_object_set(c, iface, "prototype", &value) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_run(c, membership);
                if (!(value.type == FVM_VALUE_BOOLEAN && !value.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set(c, alias, "prototype", &old) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program snapshot;
                snapshot.variable("item");
                snapshot.variable("Alias");
                snapshot.op(0x54);
                snapshot.finish();
                value = class_run(c, snapshot);
                if (!(value.type == FVM_VALUE_BOOLEAN && value.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            const unsigned char missing[] = {0x69, 0x2c, 0x54, 0x2b};
            for (unsigned i = 0; i < 4; ++i)
            {
                Program p;
                p.op(missing[i]);
                if (i < 2)
                {
                    p.number(42);
                }
                p.finish();
                value = class_run(c, p, version ? FVM_OK : FVM_ERROR);
                if (version)
                {
                    if (i < 2)
                    {
                        if (!(value.type == FVM_VALUE_NUMBER && value.number == 42))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (i == 2)
                    {
                        if (!(value.type == FVM_VALUE_BOOLEAN && !value.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(value.type == FVM_VALUE_NULL))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            (void)iface;
            (void)other;
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
