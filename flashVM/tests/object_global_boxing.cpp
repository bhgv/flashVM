/* Object primitive boxing follows current globals; host boxing keeps registry behavior. */
#include <stdio.h>
#include <stdlib.h>
#include "primitive_boxing_fixture.hpp"


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            for (unsigned kind = 0; kind < 14; ++kind)
            {
                if (version && version < 4)
                {
                    continue;
                }
                BoxingFixture f;
                boxing_init(runtime, f, version, type, kind);
                FVM_Context *c = f.context;
                FVM_Value object_class, owner_value, out;
                if (!(fvm_get_global(c, "Object", &object_class) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *owner = 0;
                if (!(fvm_object_create(c, &owner) == FVM_OK &&
                      fvm_object_set(c, owner, "Object", &object_class) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                owner_value = class_value(owner);
                if (!(fvm_set_global(c, "objectHolder", &owner_value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned route = 0; route < 16; ++route)
                {
                    unsigned mode = route < 4 ? 0 : (route - 4) % 3;
                    if (mode && !fvm_jit_available())
                    {
                        continue;
                    }
                    FVM_Result expected = boxing_expected(f);
                    FVM_Object *instance = 0;
                    f.reads = f.collections = f.blocked_collections = 0;
                    if (route < 2)
                    {
                        FVM_Result status = route
                                                ? fvm_function_construct(c, object_class.object, 1, &f.input, &instance)
                                                : fvm_construct(c, "Object", 1, &f.input, &instance);
                        if (!(status == expected && (expected == FVM_OK ? instance != 0 : instance == 0)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (route < 4)
                    {
                        FVM_Result status = route == 2
                                                ? fvm_call(c, "Object", 1, &f.input, &out)
                                                : fvm_function_invoke(c, object_class.object, 0, 1, &f.input, &out);
                        if (!(status == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (expected == FVM_OK)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            instance = out.object;
                        }
                    }
                    else
                    {
                        Program code;
                        code.variable("boxingInput");
                        code.number(1);
                        if (route >= 10)
                        {
                            code.variable("objectHolder");
                        }
                        code.text("Object");
                        code.op(route < 7 ? 0x40 : route < 10 ? 0x3d : route < 13 ? 0x53 : 0x52);
                        code.finish();
                        out = boxing_run(c, code, mode, expected);
                        if (expected == FVM_OK)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            instance = out.object;
                        }
                    }
                    if (expected == FVM_OK)
                    {
                        if (!(fvm_gc_pin(c, instance) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        boxing_check(f, instance, false);
                    }
                    else if (expected == FVM_EXCEPTION && route < 4)
                    {
                        if (!(c->exception_pending && c->exception_value.object == f.failure &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    unsigned reads = version && kind >= 6 && kind <= 9 ? 1U : 0U;
                    if (!(f.reads == reads && !f.collections))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (reads)
                    {
                        if (!(f.blocked_collections == 1))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->exception_pending &&
                          fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
