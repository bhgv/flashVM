/* Real SWF6--8 AsBroadcaster calls, nested script GC and actual JIT entries. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/broadcaster.h"


static FVM_Result collect(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(!c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* Nested script getters may enter JIT too; verify actual native entry without assuming one call. */
static FVM_Value run_broadcaster(FVM_Context *c, const Program &program, FVM_Result expected = FVM_OK)
{
    FVM_Object *fn = 0;
    FVM_JitCode *code = 0;
    FVM_Value out;
    if (!(fvm_function_create_avm1(c, program.bytes.begin(), program.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode == 1)
    {
        if (!(fvm_jit_compile(program.bytes.begin(), program.bytes.size(), &code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long entries = c->execution_mode.stats.native_entries;
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode)
    {
        if (!(c->execution_mode.stats.native_entries > entries))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(c->execution_mode.stats.native_entries == entries))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && (!code || !code->active)))
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
    if (code)
    {
        if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_unpin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *r = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < (fvm_jit_available() ? 3U : 1U); ++class_mode)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(r, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value alias;
            fvm_value_number(&alias, 17);
            if (!(fvm_set_global(c, "asbroadcaster", &alias) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_broadcaster_install(c) == (version == 6 ? FVM_INVALID_ARGUMENT : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_get_global(c, "asbroadcaster", &alias) == FVM_OK && alias.number == 17))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->global->properties.erase(fvm_detail::string("asbroadcaster"));
            if (!(fvm_standard_broadcaster_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *source = 0, *listener = 0;
            if (!(fvm_object_create(c, &source) == FVM_OK && fvm_object_create(c, &listener) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value v = class_value(source);
            if (!(fvm_set_global(c, "source", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v = class_value(listener);
            if (!(fvm_set_global(c, "listener", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_register_function(c, "collectBroadcast", collect, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->functions["collectBroadcast"].collector_entry = true;
            Program handler;
            handler.variable("this");
            handler.text("received");
            handler.variable("payload");
            handler.op(0x4f);
            handler.number(0);
            handler.text("collectBroadcast");
            handler.op(0x3d);
            handler.finish();
            const char *parameters[] = {"payload"};
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, handler.bytes.begin(), handler.bytes.size(), 1, parameters, 0, &fn) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v = class_value(fn);
            if (!(fvm_object_set(c, listener, "onValue", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program initialize;
            initialize.variable("source");
            initialize.number(1);
            initialize.variable("AsBroadcaster");
            initialize.text("initialize");
            initialize.op(0x52);
            initialize.finish();
            v = run_broadcaster(c, initialize);
            if (!(v.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program add;
            add.variable("listener");
            add.number(1);
            add.variable("source");
            add.text("addListener");
            add.op(0x52);
            add.finish();
            v = run_broadcaster(c, add);
            if (!(v.type == FVM_VALUE_BOOLEAN && v.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program send;
            send.number(42);
            send.text("onValue");
            send.number(2);
            send.variable("source");
            send.text("broadcastMessage");
            send.op(0x52);
            send.finish();
            v = run_broadcaster(c, send);
            if (!(v.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, listener, "received", &v) == FVM_OK && v.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program throwing;
            throwing.text("broadcast failure");
            throwing.op(0x2a);
            throwing.op(0);
            if (!(fvm_function_create_avm1(c, throwing.bytes.begin(), throwing.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v = class_value(fn);
            if (!(fvm_object_set(c, listener, "onFailure", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program failed;
            failed.text("onFailure");
            failed.number(1);
            failed.variable("source");
            failed.text("broadcastMessage");
            failed.op(0x52);
            failed.finish();
            run_broadcaster(c, failed, FVM_EXCEPTION);
            Program remove;
            remove.variable("listener");
            remove.number(1);
            remove.variable("source");
            remove.text("removeListener");
            remove.op(0x52);
            remove.finish();
            v = run_broadcaster(c, remove);
            if (!(v.type == FVM_VALUE_BOOLEAN && v.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v = run_broadcaster(c, remove);
            if (!(v.type == FVM_VALUE_BOOLEAN && !v.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(r);
}
