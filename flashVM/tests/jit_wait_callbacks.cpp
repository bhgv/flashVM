/* Dynamic native wait predicates retain collecting/throwing request receivers and intrinsic queries. */
#define FVM_WAIT_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_wait_frames.cpp"
#include "flashvm/movieclip.h"

static bool wait_ready, wait_throw;
static unsigned wait_conversions;


static FVM_Result wait_forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(false))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static FVM_Result wait_convert(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++wait_conversions;
    FVM_Value value;
    fvm_value_undefined(&value);
    if (!(fvm_set_global(c, "request", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result status = fvm_gc_collect(c, 0);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!(fvm_object_get(c, frame->receiver, "alive", &value) == FVM_OK && value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(out, wait_ready ? "label" : "16002");
    return wait_throw ? fvm_exception_raise(c, out) : FVM_OK;
}


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
            FVM_Object *root = 0;
            if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 3, &root) == FVM_OK &&
                  fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_accessor_rooted(c, root, "_framesloaded", wait_forbidden, 0, 0, 0) == FVM_OK &&
                  fvm_object_define_accessor_rooted(c, root, "_totalframes", wait_forbidden, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned ready = 0; ready < 2; ++ready)
            {
                for (unsigned throwing = 0; throwing < 2; ++throwing)
                {
                    FVM_Object *object = 0, *fn = 0;
                    FVM_Value value;
                    const unsigned char empty[] = {0};
                    if (!(fvm_object_create(c, &object) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 17);
                    if (!(fvm_object_set(c, object, "alive", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_define_method_rooted(c, object, "valueOf", wait_forbidden, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK &&
                          fvm_function_set_body(c, fn, wait_convert, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(fn);
                    if (!(fvm_object_set(c, object, "toString", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(object);
                    if (!(fvm_set_global(c, "request", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    wait_conversions = 0;
                    wait_ready = ready != 0;
                    wait_throw = throwing != 0;
                    Program p;
                    p.number(77);
                    p.variable("request");
                    wait_value(p, 1);
                    p.number(11);
                    p.finish();
                    FVM_Value out = class_run(c, p, throwing ? FVM_EXCEPTION : FVM_OK, root);
                    if (!(wait_conversions == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!throwing)
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == (ready ? 11 : 77)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
