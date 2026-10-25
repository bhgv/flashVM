/* isPrototypeOf projects primitive arguments through active globals before walking prototypes. */
#include <stdio.h>
#include <stdlib.h>
#include "primitive_boxing_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
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
                boxing_init(rt, f, version, type, kind);
                FVM_Context *c = f.context;
                FVM_Value factory_proto, method;
                if (!(fvm_object_get(c, f.factory, "prototype", &factory_proto) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *receiver = boxing_custom(f) ? factory_proto.object : f.prototype;
                if (!(fvm_gc_pin(c, receiver) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(receiver);
                if (!(fvm_set_global(c, "probePrototype", &value) == FVM_OK &&
                      fvm_object_get(c, receiver, "isPrototypeOf", &method) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned route = 0; route < 5; ++route)
                {
                    if (route >= 3 && !fvm_jit_available())
                    {
                        continue;
                    }
                    FVM_Value out;
                    fvm_value_number(&out, 91);
                    FVM_Result expected = boxing_expected(f);
                    f.reads = f.collections = f.blocked_collections = 0;
                    if (!route)
                    {
                        if (!(fvm_object_call(c, receiver, "isPrototypeOf", 1, &f.input, &out) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (route == 1)
                    {
                        if (!(fvm_function_invoke(c, method.object, receiver, 1, &f.input, &out) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        Program code;
                        code.variable("boxingInput");
                        code.number(1);
                        code.variable("probePrototype");
                        code.text("isPrototypeOf");
                        code.op(0x52);
                        code.finish();
                        out = boxing_run(c, code, route - 2, expected);
                    }
                    if (expected == FVM_OK)
                    {
                        if (!(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == (!!version && !boxing_fallback(f))))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (expected == FVM_EXCEPTION && route < 2)
                    {
                        if (!(c->exception_pending && c->exception_value.object == f.failure &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    unsigned reads = version && kind >= 6 && kind <= 9 ? 1U : 0U;
                    if (!(f.reads == reads && !f.collections && f.blocked_collections == reads))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->exception_pending &&
                          fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                /* Missing and nullish arguments remain false without reading boxing globals. */
                for (unsigned count = 0; count < 3; ++count)
                {
                    FVM_Value input, out;
                    fvm_value_undefined(&input);
                    if (count == 2)
                    {
                        input.type = FVM_VALUE_NULL;
                    }
                    f.reads = 0;
                    if (!(fvm_object_call(c, receiver, "isPrototypeOf", count ? 1 : 0, &input, &out) == FVM_OK &&
                          out.type == FVM_VALUE_BOOLEAN && !out.boolean && !f.reads))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
