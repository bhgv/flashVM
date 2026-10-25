/* Case-aware installation, late-load gates, lifecycle guards and ignored coercible arguments. */
#include <stdio.h>
#include <stdlib.h>
#include "timers_fixture.hpp"
#include "flashvm/stage.h"
#include "flashvm/mouse.h"

static unsigned conversions;


static FVM_Result convert(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++conversions;
    return FVM_ERROR;
}


static FVM_Result with_argument(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Value *arg = (FVM_Value *)raw;
    if (!(c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_call(c, "updateAfterEvent", 1, arg, out);
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned mixed = 0; mixed < 2; ++mixed)
        {
            for (unsigned slot = 0; slot < 2; ++slot)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const char *name = mixed ? "UPDATEAFTEREVENT" : "updateAfterEvent";
                FVM_Value v = timer_number(77);
                if (!slot)
                {
                    if (!(fvm_set_global(c, name, &v) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_register_function(c, name, timer_probe, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_standard_install(c) == (!mixed || version < 7 ? FVM_INVALID_ARGUMENT : FVM_OK)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!slot)
                {
                    if (!(fvm_get_global(c, name, &v) == FVM_OK && v.number == 77))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(c->functions[name].callback == timer_probe))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_context_destroy(c);
            }
        }
    }
    FVM_Context *c = timer_context(rt, 0);
    FVM_Value fn;
    if (!(fvm_get_global(c, "updateAfterEvent", &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 4, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_function_invoke(c, fn.object, 0, 0, 0, &out) == FVM_UNSUPPORTED && out.type == FVM_VALUE_UNDEFINED &&
          !c->stage_update_pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    c = timer_context(rt);
    FVM_Object *arg = 0, *listener = 0;
    if (!(fvm_object_create(c, &arg) == FVM_OK && fvm_object_create(c, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, arg, "toString", convert, 0, 0) == FVM_OK &&
          fvm_object_define_method_rooted(c, arg, "valueOf", convert, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value = timer_object(arg);
    timer_call(c, "updateAfterEvent", 1, &value);
    if (!(!c->stage_update_pending && !conversions))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, listener, "onMouseMove", with_argument, &value, arg) == FVM_OK &&
          fvm_input_add_mouse_listener(c, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, 1, 2) == FVM_OK && c->stage_update_pending &&
          !c->stage_update_allowed && !conversions))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned phase = 0; phase < 3; ++phase)
    {
        if (!phase)
        {
            c->gc_collecting = true;
        }
        else if (phase == 1)
        {
            c->gc_releasing = true;
        }
        else
        {
            c->destroying = true;
        }
        timer_call(c, "updateAfterEvent", 0, 0, FVM_INVALID_ARGUMENT);
        c->gc_collecting = c->gc_releasing = c->destroying = false;
        if (!(c->stage_update_pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
