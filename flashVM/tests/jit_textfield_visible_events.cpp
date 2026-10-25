/* Actual SWF6–8 text SetMember and replaceText/setTextFormat CallMethod execute observers in all JIT modes. */
#define FVM_SCROLL_EVENTS_TRANSFER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_events_transfer.cpp"
#include "textfield_edit_events_fixture.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
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
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            for (unsigned edit = 0; edit < 3; ++edit)
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
                FVM_Object *field = scroll_field(c);
                if (edit == 2)
                {
                    FVM_Value policy;
                    fvm_value_boolean(&policy, 1);
                    if (!(fvm_object_set(c, field, "wordWrap", &policy) == FVM_OK &&
                          fvm_object_set(c, field, "autoSize", &policy) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    scroll_number(c, field, "_width", 60);
                    scroll_number(c, field, "_height", 200);
                }
                FVM_Object *listener = script_listener(c);
                if (!(scroll_listen(c, field, listener)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value v;
                if (!(fvm_object_get(c, listener, "onScroller", &v) == FVM_OK &&
                      fvm_object_set(c, field, "onScroller", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&v, 0);
                if (!(fvm_object_set(c, field, "seen", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                v = class_value(field);
                if (!(fvm_set_global(c, "field", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *format = edit_format(c, 20);
                v = class_value(format);
                if (!(fvm_set_global(c, "format", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program p;
                if (edit == 0)
                {
                    p.variable("field");
                    p.text("text");
                    p.text("A");
                    p.op(0x4f);
                }
                else if (edit == 1)
                {
                    p.text("A");
                    p.variable("field");
                    p.text("length");
                    p.op(0x4e);
                    p.number(0);
                    p.number(3);
                    p.variable("field");
                    p.text("replaceText");
                    p.op(0x52);
                    p.op(0x17);
                }
                else
                {
                    p.variable("format");
                    p.number(1);
                    p.variable("field");
                    p.text("setTextFormat");
                    p.op(0x52);
                    p.op(0x17);
                }
                p.number(91);
                p.finish();
                FVM_Object *fn = 0;
                FVM_JitCode *code = 0;
                if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
                      fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_execution_set_mode(c,
                                             mode == 0   ? FVM_EXECUTION_INTERPRETER
                                             : mode == 1 ? FVM_EXECUTION_MANUAL
                                                         : FVM_EXECUTION_JIT,
                                             0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (mode == 1)
                {
                    if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_ExecutionStats before, after;
                before.struct_size = sizeof(before);
                after.struct_size = sizeof(after);
                if (!(fvm_execution_get_stats(c, &before) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_invoke(c, fn, 0, 0, 0, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER &&
                      v.number == 91 && fvm_execution_get_stats(c, &after) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(mode ? after.native_entries > before.native_entries
                           : after.interpreter_entries > before.interpreter_entries &&
                                 after.native_entries == before.native_entries))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_TextFieldScrollInfo info = scroll_info(c, field);
                if (!(info.scroll == 1 && !info.hscroll && info.maxscroll == 1 && !info.maxhscroll &&
                      info.bottom_scroll == (edit == 2 ? 3UL : 1UL)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(counted_metric(c, field, "seen") == 1 && counted_metric(c, listener, "seen") == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, listener, "source", &v) == FVM_OK && v.object == field))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_invoke(c, fn, 0, 0, 0, &v) == FVM_OK && counted_metric(c, field, "seen") == 1 &&
                      counted_metric(c, listener, "seen") == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK &&
                      fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
                      !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
