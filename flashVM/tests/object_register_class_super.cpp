/* Loaded registerClass rejects super before coercing its name or replacing a constructor. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "object_super_accessors.cpp"
#include "flashvm/exception.h"

static unsigned registration_names, registration_fault;


static FVM_Result registration_name(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++registration_names;
    if (registration_fault)
    {
        fvm_value_number(out, 71);
        return fvm_exception_raise(c, out);
    }
    return fvm_value_string_bytes(out, "Class\0Name", 10);
}


static FVM_Value registration_object(FVM_Object *o)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
    return value;
}
#ifdef FVM_REGISTER_SUPER_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#endif


static FVM_Result registration_call(FVM_Context *c, FVM_Object *klass, FVM_Value *args, FVM_Value *out)
{
#ifdef FVM_REGISTER_SUPER_JIT
    FVM_Value value = registration_object(klass);
    if (!(fvm_set_global(c, "registrationObject", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "registrationName", args) == FVM_OK &&
          fvm_set_global(c, "registrationConstructor", args + 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program call;
    call.variable("registrationConstructor");
    call.variable("registrationName");
    call.number(2);
    call.variable("registrationObject");
    call.text("registerClass");
    call.op(0x52);
    call.finish();
    *out = class_run(c, call);
    return FVM_OK;
#else
    return fvm_object_call(c, klass, "registerClass", 2, args, out);
#endif
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
#ifdef FVM_REGISTER_SUPER_JIT
    if (!fvm_jit_available())
    {
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
        for (unsigned version = 0; version <= 8; ++version)
        {
            if (version && version < 5)
            {
                continue;
            }
#ifdef FVM_REGISTER_SUPER_JIT
            if (!version)
            {
                continue;
            }
#endif
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
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
            FVM_Object *instance = 0, *target = 0, *facade = 0, *key = 0, *name_fn = 0, *found = 0;
            super_accessor_setup(c, &instance, &target, &facade);
            FVM_Value klass, args[2], out;
            if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string_bytes(args, "Class\0Name", 10);
            args[1] = klass;
            if (!(registration_call(c, klass.object, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                const unsigned char end[] = {0};
                if (!(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK && fvm_gc_pin(c, key) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &name_fn) == FVM_OK &&
                      fvm_function_set_body(c, name_fn, registration_name, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = registration_object(name_fn);
                if (!(fvm_object_set(c, key, "toString", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                args[0] = registration_object(key);
            }
            registration_names = registration_fault = 0;
            args[1] = registration_object(facade);
            if (!(registration_call(c, klass.object, args, &out) == FVM_OK && !!out.boolean == !version &&
                  registration_names == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_registered_class_bytes(c, "Class\0Name", 10, &found) == FVM_OK &&
                  found == (version ? klass.object : facade)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                registration_fault = 1;
                if (!(registration_call(c, klass.object, args, &out) == FVM_OK && !out.boolean && !registration_names &&
                      !c->exception_pending))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                args[1] = klass;
                if (!(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_EXCEPTION &&
                      registration_names == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 71 && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_registered_class_bytes(c, "Class\0Name", 10, &found) == FVM_OK && found == klass.object))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                registration_fault = 0;
                if (!(registration_call(c, klass.object, args, &out) == FVM_OK && out.boolean &&
                      registration_names == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_value_undefined(args + 1);
            args[1].type = FVM_VALUE_NULL;
            if (!(registration_call(c, klass.object, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_registered_class_bytes(c, "Class\0Name", 10, &found) == FVM_NOT_FOUND))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    fvm_runtime_destroy(rt);
}
