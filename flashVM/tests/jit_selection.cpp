/* Real Selection calls and keyboard script callbacks across SWF6--8 interpreter/JIT modes. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"


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
static FVM_Value run_selection(FVM_Context *c, const Program &program, FVM_Result expected = FVM_OK)
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
    if (!(fvm_jit_compile(program.bytes.begin(), program.bytes.size(), &code) == FVM_OK))
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
    if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !code->active))
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
    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
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
    for (unsigned version = 6; version <= 8; ++version)
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
            FVM_Value alias;
            fvm_value_number(&alias, 77);
            if (!(fvm_set_global(c, "selection", &alias) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_selection_install(c) == (version == 6 ? FVM_INVALID_ARGUMENT : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_get_global(c, "selection", &alias) == FVM_OK && alias.number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->global->properties.erase("selection");
            if (!(fvm_standard_selection_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = 0;
            if (!(fvm_movieclip_create(c, "movie", 1, &clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value = class_value(clip);
            if (!(fvm_set_global(c, "movie", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_register_function(c, "collectFocus", collect, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->functions["collectFocus"].collector_entry = true;
            Program getter;
            getter.number(0);
            getter.text("collectFocus");
            getter.op(0x3d);
            getter.finish();
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, getter.bytes.begin(), getter.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value args[3];
            fvm_value_string(args, "focusEnabled");
            args[1] = class_value(fn);
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_NULL;
            if (!(fvm_object_call(c, clip, "addProperty", 3, args, &value) == FVM_OK && value.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *observer = 0;
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
            Program observe;
            observe.variable("this");
            observe.text("old");
            observe.variable("old");
            observe.op(0x4f);
            observe.variable("this");
            observe.text("next");
            observe.variable("next");
            observe.op(0x4f);
            observe.number(0);
            observe.text("collectFocus");
            observe.op(0x3d);
            observe.finish();
            const char *observer_parameters[] = {"old", "next"};
            FVM_Object *observer_handler = 0;
            if (!(fvm_function_create_avm1(
                      c, observe.bytes.begin(), observe.bytes.size(), 2, observer_parameters, 0, &observer_handler) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(observer_handler);
            if (!(fvm_object_set(c, observer, "onSetFocus", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program add_observer;
            add_observer.variable("observer");
            add_observer.number(1);
            add_observer.variable("Selection");
            add_observer.text("addListener");
            add_observer.op(0x52);
            add_observer.finish();
            value = run_selection(c, add_observer);
            if (!(value.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program set;
            set.variable("movie");
            set.number(1);
            set.variable("Selection");
            set.text("setFocus");
            set.op(0x52);
            set.finish();
            value = run_selection(c, set);
            if (!(value.type == FVM_VALUE_BOOLEAN && value.boolean && c->keyboard_focus == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "old", &value) == FVM_OK && value.type == FVM_VALUE_NULL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "next", &value) == FVM_OK && value.object == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program get;
            get.number(0);
            get.variable("Selection");
            get.text("getFocus");
            get.op(0x52);
            get.finish();
            value = run_selection(c, get);
            if (!(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "_level0")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Real AVM1 focus callbacks collect, preserve receiver and receive the old/new object. */
            Program focus;
            focus.variable("this");
            focus.text("previous");
            focus.variable("previous");
            focus.op(0x4f);
            focus.number(0);
            focus.text("collectFocus");
            focus.op(0x3d);
            focus.finish();
            const char *parameters[] = {"previous"};
            FVM_Object *focus_handler = 0;
            if (!(fvm_function_create_avm1(
                      c, focus.bytes.begin(), focus.bytes.size(), 1, parameters, 0, &focus_handler) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(focus_handler);
            if (!(fvm_object_set(c, clip, "onSetFocus", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set(c, clip, "onKillFocus", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = run_selection(c, set);
            if (!(value.boolean && c->keyboard_focus == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, clip, "previous", &value) == FVM_OK && value.type == FVM_VALUE_NULL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program clear;
            clear.op(0x96);
            clear.op(1);
            clear.op(0);
            clear.op(2); /* Raw Push null. */
            clear.number(1);
            clear.variable("Selection");
            clear.text("setFocus");
            clear.op(0x52);
            clear.finish();
            value = run_selection(c, clear);
            if (!(value.boolean && !c->keyboard_focus))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "old", &value) == FVM_OK && value.object == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, observer, "next", &value) == FVM_OK && value.type == FVM_VALUE_NULL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, clip, "previous", &value) == FVM_OK && value.type == FVM_VALUE_NULL))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = run_selection(c, set);
            if (!(value.boolean && c->keyboard_focus == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program remove_observer;
            remove_observer.variable("observer");
            remove_observer.number(1);
            remove_observer.variable("Selection");
            remove_observer.text("removeListener");
            remove_observer.op(0x52);
            remove_observer.finish();
            value = run_selection(c, remove_observer);
            if (!(value.type == FVM_VALUE_BOOLEAN && value.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->selection_listeners.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program callback;
            callback.variable("this");
            callback.text("seen");
            callback.number(99);
            callback.op(0x4f);
            callback.number(1);
            callback.finish();
            FVM_Object *handler = 0;
            if (!(fvm_function_create_avm1(c, callback.bytes.begin(), callback.bytes.size(), 0, 0, 0, &handler) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(handler);
            if (!(fvm_object_set(c, clip, "onKeyDown", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_KeyboardState state;
            std::memset(&state, 0, sizeof(state));
            state.struct_size = sizeof(state);
            state.last_code = FVM_KEY_LEFT;
            state.down[FVM_KEY_LEFT] = 1;
            if (!(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, clip, "seen", &value) == FVM_OK && value.number == 99))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Real script exceptions from observers stop delivery after focus publication. */
            Program throwing;
            throwing.text("focus failure");
            throwing.op(0x2a);
            throwing.op(0);
            FVM_Object *throw_handler = 0;
            if (!(fvm_function_create_avm1(c, throwing.bytes.begin(), throwing.bytes.size(), 0, 0, 0, &throw_handler) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(throw_handler);
            if (!(fvm_object_set(c, observer, "onSetFocus", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            run_selection(c, add_observer);
            run_selection(c, clear, FVM_EXCEPTION);
            if (!(!c->keyboard_focus && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = run_selection(c, remove_observer);
            if (!(value.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_set_keyboard_focus(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
