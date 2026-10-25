/* Real Mouse registration and event handlers under SWF6--8 interpretation/manual/automatic JIT. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#include "flashvm/input_queue.h"
#include "flashvm/movieclip.h"


static FVM_Result collect_mouse(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Object *script(FVM_Context *c, const Program &p, FVM_JitCode **code, const char *parameter = 0)
{
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(
              c, p.bytes.begin(), p.bytes.size(), parameter ? 1 : 0, parameter ? &parameter : 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode == 1)
    {
        if (!(fvm_function_set_body(c, fn, fvm_jit_body, *code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return fn;
}


static FVM_Value invoke(FVM_Context *c, FVM_Object *fn)
{
    FVM_Value out;
    unsigned long entries = c->execution_mode.stats.native_entries;
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
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
    return out;
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
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
            if (version < 6)
            {
                if (!(fvm_standard_mouse_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value mouse, out;
                if (!(fvm_get_global(c, "Mouse", &mouse) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, mouse.object, "hide", 0, 0, &out) == FVM_OK && out.number == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, mouse.object, "addListener", 0, 0, &out) == FVM_UNSUPPORTED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
                continue;
            }
            FVM_Value value;
            fvm_value_number(&value, 77);
            if (!(fvm_set_global(c, "mouse", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_mouse_install(c) == (version == 6 ? FVM_INVALID_ARGUMENT : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->global->properties.erase("mouse");
            if (!(fvm_standard_mouse_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = 0, *observer = 0;
            if (!(fvm_movieclip_create(c, "movie", 1, &clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(clip);
            if (!(fvm_set_global(c, "movie", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_create(c, &observer) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(observer);
            if (!(fvm_set_global(c, "observer", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_register_function(c, "collectMouse", collect_mouse, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->functions["collectMouse"].collector_entry = true;
            Program callback;
            callback.number(0);
            callback.text("collectMouse");
            callback.op(0x3d);
            callback.op(0x17);
            callback.variable("this");
            callback.text("pointerX");
            callback.variable("movie");
            callback.text("_xmouse");
            callback.op(0x4e);
            callback.op(0x4f);
            callback.number(99);
            callback.finish();
            FVM_JitCode *handler_code = 0;
            FVM_Object *handler = script(c, callback, &handler_code);
            value = class_value(handler);
            if (!(fvm_object_set(c, observer, "onMouseDown", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set(c, observer, "onMouseMove", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set(c, observer, "onMouseUp", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program add;
            add.variable("observer");
            add.number(1);
            add.variable("Mouse");
            add.text("addListener");
            add.op(0x52);
            add.finish();
            FVM_JitCode *add_code = 0;
            FVM_Object *add_fn = script(c, add, &add_code);
            if (!(invoke(c, add_fn).type == FVM_VALUE_UNDEFINED && c->mouse_listeners.size() == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const FVM_MouseEventType types[] = {FVM_MOUSE_EVENT_DOWN, FVM_MOUSE_EVENT_MOVE, FVM_MOUSE_EVENT_UP};
            for (unsigned i = 0; i < 3; ++i)
            {
                unsigned long entries = c->execution_mode.stats.native_entries;
                if (!(fvm_input_dispatch_mouse(c, types[i], 30 + i, -4) == FVM_OK))
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
                if (!(fvm_object_get(c, observer, "pointerX", &value) == FVM_OK && value.number == 30 + i))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                entries = c->execution_mode.stats.native_entries;
                if (!(fvm_input_enqueue_mouse(c, types[i], 40 + i, -5) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned processed = 99;
                if (!(fvm_input_pump(c, 0, &processed) == FVM_OK && processed == 1))
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
                if (!(fvm_object_get(c, observer, "pointerX", &value) == FVM_OK && value.number == 40 + i))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program wheel;
            wheel.number(0);
            wheel.text("collectMouse");
            wheel.op(0x3d);
            wheel.op(0x17);
            wheel.variable("this");
            wheel.text("wheelDelta");
            wheel.variable("delta");
            wheel.op(0x4f);
            wheel.variable("this");
            wheel.text("pointerX");
            wheel.variable("movie");
            wheel.text("_xmouse");
            wheel.op(0x4e);
            wheel.op(0x4f);
            wheel.finish();
            FVM_JitCode *wheel_code = 0;
            FVM_Object *wheel_fn = script(c, wheel, &wheel_code, "delta");
            value = class_value(wheel_fn);
            if (!(fvm_object_set(c, observer, "onMouseWheel", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned long wheel_entries = c->execution_mode.stats.native_entries;
            if (!(fvm_input_dispatch_mouse_wheel(c, 2.5, 90, 91) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (class_mode)
            {
                if (!(c->execution_mode.stats.native_entries > wheel_entries))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_object_get(c, observer, "wheelDelta", &value) == FVM_OK && value.number == 2.5))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "pointerX", &value) == FVM_OK && value.number == 90))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            wheel_entries = c->execution_mode.stats.native_entries;
            if (!(fvm_input_enqueue_mouse_wheel(c, -1.25, 92, 93) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned wheel_processed = 99;
            if (!(fvm_input_pump(c, 0, &wheel_processed) == FVM_OK && wheel_processed == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (class_mode)
            {
                if (!(c->execution_mode.stats.native_entries > wheel_entries))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_object_get(c, observer, "wheelDelta", &value) == FVM_OK && value.number == -1.25))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "pointerX", &value) == FVM_OK && value.number == 92))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program throwing;
            throwing.text("pointer failure");
            throwing.op(0x2a);
            throwing.op(0);
            FVM_JitCode *throw_code = 0;
            FVM_Object *throw_fn = script(c, throwing, &throw_code);
            value = class_value(throw_fn);
            if (!(fvm_object_set(c, observer, "onMouseUp", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_UP, 42, 43) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->pointer_world_x == 42 && c->pointer_world_y == 43 && fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_UP, 80, 81) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, 82, 83) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned processed = 99, pending = 99;
            if (!(fvm_input_pump(c, 0, &processed) == FVM_EXCEPTION && processed == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->pumping_input && fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->pointer_world_x == 80 && fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_pump(c, 0, &processed) == FVM_OK && processed == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "pointerX", &value) == FVM_OK && value.number == 82))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(throw_fn);
            if (!(fvm_object_set(c, observer, "onMouseWheel", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_dispatch_mouse_wheel(c, 1, 101, 102) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->pointer_world_x == 101 && fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_enqueue_mouse_wheel(c, -1, 103, 104) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, 105, 106) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_pump(c, 0, &processed) == FVM_EXCEPTION && !processed))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->pumping_input && !c->mouse_dispatching && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->pointer_world_x == 103 && fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK && fvm_input_pump(c, 0, &processed) == FVM_OK && processed == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->pointer_world_x == 105))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, wheel_fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(wheel_code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, handler, 0, 0, 0) == FVM_OK && fvm_jit_destroy(handler_code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, add_fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(add_code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, throw_fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(throw_code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
