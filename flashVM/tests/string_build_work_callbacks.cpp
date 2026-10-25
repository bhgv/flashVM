/* Builder argument coercions retain order, shared work and native GC barriers. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "string_loaded_work.cpp"
#include "flashvm/exception.h"

static FVM_Object *inner;
static unsigned calls, failure;


static FVM_Result build_callback(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    if (!(++calls == id))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == id)
    {
        fvm_value_number(out, 81);
        return fvm_exception_raise(c, out);
    }
    if (id == 1)
    {
        FVM_Value arg, nested;
        fvm_value_string(&arg, "z");
        FVM_Result r = fvm_object_call(c, inner, "indexOf", 1, &arg, &nested);
        if (r != FVM_OK)
        {
            return r;
        }
    }
    return fvm_value_string(out, "65");
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        string_work_load(c, version);
        FVM_Object *str = string_work_make(c, "x", 1), *objects[2];
        inner = string_work_make(c, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", 40);
        unsigned ids[] = {1, 2};
        FVM_Value args[2], out, klass;
        if (!(fvm_get_global(c, "String", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            if (!(fvm_construct(c, "Object", 0, 0, objects + i) == FVM_OK && fvm_gc_pin(c, objects[i]) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(c, objects[i], "toString", build_callback, ids + i, 0) == FVM_OK &&
                  fvm_object_define_method_rooted(c, objects[i], "valueOf", build_callback, ids + i, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            args[i] = object(objects[i]);
        }
        const char *names[] = {"concat", "fromCharCode", "slice", "substring", "substr"};
        for (unsigned method = 0; method < 5; ++method)
        {
            FVM_Object *self = method == 1 ? klass.object : str;
            calls = 0;
            failure = 0;
            rt->config.instruction_limit = 16;
            if (!(fvm_object_call(c, self, names[method], 2, args, &out) == FVM_ERROR && calls == 1 &&
                  c->avm1_steps == 16))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            string_work_clean(c);
            rt->config.instruction_limit = 0;
            for (failure = 0; failure < 3; ++failure)
            {
                calls = 0;
                FVM_Result r = fvm_object_call(c, self, names[method], 2, args, &out);
                if (!(calls == (failure ? failure : 2)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (failure)
                {
                    if (!(r == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(r == FVM_OK && out.type == FVM_VALUE_STRING))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (method == 0)
                    {
                        if (!(!std::strcmp(out.string, "x6565")))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (method == 1)
                    {
                        if (!(!std::strcmp(out.string, "AA")))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_value_string_size(&out) == 0))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                string_work_clean(c);
            }
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
