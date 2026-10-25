/* Input events share one real AVM1 instruction budget across interpreter/manual/automatic JIT. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/movieclip.h"


struct Trace
{
    unsigned calls;
};


static FVM_Result mark(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    ++((Trace *)raw)->calls;
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result deliver(FVM_Context *c, unsigned kind, FVM_Object *ns)
{
    if (!kind)
    {
        FVM_KeyboardState state = c->keyboard;
        state.last_code = FVM_KEY_SPACE;
        state.down[FVM_KEY_SPACE] = 1;
        return fvm_input_dispatch_keyboard(c, &state, 1);
    }
    if (kind == 1)
    {
        return fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_DOWN, 17, 31);
    }
    if (kind == 2)
    {
        return fvm_input_dispatch_mouse_wheel(c, 1, 17, 31);
    }
    FVM_Value arg, out;
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_NULL;
    return fvm_object_call(c, ns, "setFocus", 1, &arg, &out);
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
    const unsigned long ordinary = rt->config.instruction_limit;
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (unsigned mode = 0; mode < (fvm_jit_available() ? 3U : 1U); ++mode)
        {
            for (unsigned kind = 0; kind < 4; ++kind)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
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
                if (!(fvm_standard_selection_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value ns;
                if (!(fvm_get_global(c, "Selection", &ns) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *clip = 0;
                if (!(fvm_movieclip_create(c, "oldFocus", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Trace trace;
                trace.calls = 0;
                if (!(fvm_register_function(c, "markInput", mark, &trace) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program handler;
                handler.number(0);
                handler.text("markInput");
                handler.op(0x3d);
                handler.op(0x17);
                for (unsigned i = 0; i < 20; ++i)
                {
                    handler.number(i);
                    handler.op(0x17);
                }
                handler.finish();
                FVM_Object *fn = 0;
                FVM_JitCode *code = 0;
                if (!(fvm_function_create_avm1(c, handler.bytes.begin(), handler.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_pin(c, fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (mode == 1)
                {
                    if (!(fvm_jit_compile(handler.bytes.begin(), handler.bytes.size(), &code) == FVM_OK))
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
                const char *name = !kind       ? "onKeyDown"
                                   : kind == 1 ? "onMouseDown"
                                   : kind == 2 ? "onMouseWheel"
                                               : "onSetFocus";
                for (unsigned i = 0; i < 4; ++i)
                {
                    FVM_Object *o = 0;
                    if (!(fvm_object_create(c, &o) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value v = class_value(fn);
                    if (!(fvm_object_set(c, o, name, &v) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!kind)
                    {
                        if (!(fvm_input_add_key_listener(c, o) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (kind < 3)
                    {
                        if (!(fvm_input_add_mouse_listener(c, o) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        v = class_value(o);
                        FVM_Value out;
                        if (!(fvm_object_call(c, ns.object, "addListener", 1, &v, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                if (!(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (kind == 3)
                {
                    if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                unsigned long entries = c->execution_mode.stats.native_entries;
                rt->config.instruction_limit = 75;
                if (!(deliver(c, kind, ns.object) == FVM_ERROR && trace.calls > 0 && trace.calls < 4))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(mode ? c->execution_mode.stats.native_entries > entries
                           : c->execution_mode.stats.native_entries == entries))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth &&
                      (!code || !code->active)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                trace.calls = 0;
                rt->config.instruction_limit = ordinary;
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (kind == 3)
                {
                    if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(deliver(c, kind, ns.object) == FVM_OK && trace.calls == 4))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (code)
                {
                    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_jit_destroy(code) == FVM_OK))
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
