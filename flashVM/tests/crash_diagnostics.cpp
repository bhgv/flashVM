/* VM faults retain mixed-engine source/argument metadata and live debugger views. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/crash.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "../src/avm1/handlers/script_call.hpp"
#include "../src/jit/internal.hpp"
#include <cstdio>
#include <cstring>

static unsigned callbacks;
static bool forced;
static FVM_Value input;


static FVM_Result fail(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(&input, 99);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 123);
    return fvm_exception_raise(c, out);
}


static void paused(FVM_Context *c, FVM_Result result, void *)
{
    ++callbacks;
    if (!(result == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value arg;
    if (!(fvm_crash_argument(c, 0, 0, &arg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(arg.type == FVM_VALUE_NUMBER && arg.number == (forced ? 42 : 99)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_configure(c, FVM_CRASH_NONE, 0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_clear(c) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *name;
    size_t n;
    bool found = false;
    for (size_t i = 0; fvm_crash_local(c, 0, i, &name, &n, &arg) == FVM_OK; ++i)
    {
        if (n == 5 && !std::memcmp(name, "input", 5))
        {
            found = true;
        }
    }
    if (!(found))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void registered(FVM_Context *c, FVM_Result result, void *)
{
    ++callbacks;
    if (!(result == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    if (!(fvm_crash_argument(c, 0, 0, &v) == FVM_OK && v.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_register(c, 0, 1, &v) == FVM_OK && v.number == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *name;
    size_t n;
    bool found = false;
    for (size_t i = 0; fvm_crash_local(c, 0, i, &name, &n, &v) == FVM_OK; ++i)
    {
        if (n == 5 && !std::memcmp(name, "input", 5))
        {
            found = true;
            if (!(v.number == 99))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(found == forced))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result stopped(FVM_Context *c, const FVM_DebugBreakpoint *, void *)
{
    ++callbacks;
    size_t count;
    if (!(fvm_debug_frame_count(c, &count) == FVM_OK && count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_DebugFrame frame;
    if (!(fvm_debug_frame_at(c, 0, &frame) == FVM_OK && frame.argc == 1 &&
          frame.engine == FVM_CRASH_ENGINE_INTERPRETER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.forced == (int)forced && frame.preservation_status == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    if (!(fvm_crash_argument(c, 0, 0, &value) == FVM_OK && value.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *name;
    size_t n;
    bool found = false;
    for (size_t i = 0; fvm_crash_local(c, 0, i, &name, &n, &value) == FVM_OK; ++i)
    {
        if (n == 5 && !std::memcmp(name, "input", 5))
        {
            found = true;
        }
    }
    if (!(found))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_OK;
}


static FVM_Result host_body(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    fvm_value_number(out, 88);
    return fvm_exception_raise(c, out);
}


static void body(FVM_ScriptFunction &fn, const char *name, const unsigned char *data, size_t n)
{
    fn.name = name;
    fn.body.assign(data, data + n);
    fn.parameters.push_back(fvm_detail::string("input"));
    fn.parameter_registers.push_back(0);
}


int main(int argc, char **argv)
{
    const unsigned char call[] = {0x96, 5, 0, 7, 0, 0, 0, 0, 0x96, 7, 0, 0, 'f', 'a', 'u', 'l', 't', 0, 0x3d, 0x3e, 0};
    const unsigned char throwing[] = {0x96, 7, 0, 0, 'i', 'n', 'p', 'u', 't', 0, 0x1c, 0x2a, 0};
    const unsigned char outer_body[] = {0x96, 7, 0, 0,    'i', 'n', 'p', 'u', 't', 0,   0x1c, 0x96, 5, 0,    7,    1,
                                        0,    0, 0, 0x96, 7,   0,   0,   'i', 'n', 'n', 'e',  'r',  0, 0x3d, 0x3e, 0};
    FVM_Config config;
    FVM_Runtime *rt;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *path = argc > 1 ? argv[1] : "crash-diagnostics.zip";
    for (unsigned engine = 0; engine < 2; ++engine)
    {
        for (unsigned detail = 0; detail < 2; ++detail)
        {
            FVM_Context *c;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, engine ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *native;
            FVM_Value ref, out;
            if (!(fvm_function_create_native(c, fail, 0, 0, &native) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            native->invocable.collector_entry = true;
            fvm_value_undefined(&ref);
            ref.type = FVM_VALUE_OBJECT;
            ref.object = native;
            if (!(fvm_set_global(c, "fault", &ref) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_ScriptFunction caller;
            body(caller, "caller", call, sizeof(call));
            forced = detail != 0;
            callbacks = 0;
            if (!(fvm_crash_set_detail(c, forced ? FVM_CRASH_DETAIL_FORCE : FVM_CRASH_DETAIL_NATURAL) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_crash_configure(c, FVM_CRASH_DEBUGGER, 0, paused, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&input, 42);
            FVM_GCRootFrame *roots = c->gc_frames;
            if (!(fvm_avm1_call_script(c, caller.name, caller, 1, &input, c->global, &out) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(callbacks == 1 && !c->crash.active && c->gc_frames == roots))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_CrashFrame frame;
            if (!(fvm_crash_frame(c, 0, &frame) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(frame.forced == (int)forced && frame.jit == (engine && fvm_jit_available())))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(frame.function_size == 6 && !std::memcmp(frame.function, "caller", 6)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(frame.bytecode_size == sizeof(call) && !std::memcmp(frame.bytecode, call, sizeof(call))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_crash_argument(c, 0, 0, &out) == FVM_NOT_FOUND))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 123))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK && fvm_crash_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Ordinary breakpoints can inspect live variables without producing crash reports. */
            if (!(fvm_crash_configure(c, FVM_CRASH_NONE, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_debug_set_breakpoint(c, "caller", 0, 1) == FVM_OK &&
                  fvm_debug_set_breakpoint_handler(c, stopped, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&input, 42);
            callbacks = 0;
            if (!(fvm_avm1_call_script(c, caller.name, caller, 1, &input, c->global, &out) == FVM_EXCEPTION &&
                  callbacks == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_debug_set_breakpoint(c, "caller", 0, 0) == FVM_OK &&
                  fvm_debug_set_breakpoint_handler(c, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Real private register parameters are available by name only in explicit FORCE mode. */
            const unsigned char regbody[] = {0x96, 5, 0,    7,    99, 0, 0, 0, 0x87, 1,
                                             0,    1, 0x17, 0x96, 2,  0, 4, 1, 0x2a, 0};
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_ScriptFunction registers;
            body(registers, "registers", regbody, sizeof(regbody));
            registers.function2 = true;
            registers.register_count = 3;
            registers.parameter_registers[0] = 1;
            if (!(fvm_crash_configure(c, FVM_CRASH_DEBUGGER, 0, registered, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&input, 42);
            callbacks = 0;
            if (!(fvm_avm1_call_script(c, registers.name, registers, 1, &input, c->global, &out) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(callbacks == 1 && !c->crash.active && c->gc_frames == roots))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Persistent reports survive generated-code destruction and collector execution. */
            if (!(fvm_crash_configure(c, FVM_CRASH_BACKTRACE, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *inner;
            const char *names[] = {"input"};
            if (!(fvm_function_create_avm1(c, throwing, sizeof(throwing), 1, names, 0, &inner) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ref.object = inner;
            if (!(fvm_set_global(c, "inner", &ref) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_ScriptFunction outer;
            body(outer, "outer", outer_body, sizeof(outer_body));
            if (!(fvm_debug_map_action(c, "outer", 0, "example.as", 17) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&input, 42);
            if (!(fvm_avm1_call_script(c, outer.name, outer, 1, &input, c->global, &out) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            size_t count;
            FVM_Result status, diagnostic;
            if (!(fvm_crash_info(c, &count, &status, &diagnostic) == FVM_OK && count == 2 && status == FVM_EXCEPTION &&
                  diagnostic == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_crash_frame(c, 1, &frame) == FVM_OK && frame.function_size == 5 &&
                  !std::memcmp(frame.function, "outer", 5)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(frame.line == 17 && frame.source_size == 10 && !std::memcmp(frame.source, "example.as", 10)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_crash_configure(c, FVM_CRASH_NONE, 0, 0, 0) == FVM_OK && fvm_crash_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_avm1_call_script(c, outer.name, outer, 1, &input, c->global, &out) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_crash_info(c, &count, &status, &diagnostic) == FVM_NOT_FOUND && count == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* ZIP modes differ only in whether active machine bytes are included. */
            for (unsigned code = 0; code < 2; ++code)
            {
                if (!(fvm_crash_configure(c, code ? FVM_CRASH_ZIP_CODE : FVM_CRASH_ZIP, path, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_avm1_call_script(c, outer.name, outer, 1, &input, c->global, &out) == FVM_EXCEPTION))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_crash_info(c, &count, &status, &diagnostic) == FVM_OK && diagnostic == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const void *data;
                size_t size;
                if (!(fvm_crash_data(c, 1, &data, &size) == FVM_OK && size > 4 && !std::memcmp(data, "PK\003\004", 4)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_crash_frame(c, 0, &frame) == FVM_OK &&
                      (frame.machine_code_size != 0) == (code && engine && fvm_jit_available())))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_crash_frame(c, 0, &frame) == FVM_OK &&
                  frame.bytecode_size == sizeof(throwing)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_crash_configure(c, FVM_CRASH_BACKTRACE, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, inner, host_body, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_invoke(c, inner, 0, 1, &input, &out) == FVM_EXCEPTION))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_CrashEngine actual;
            if (!(fvm_crash_frame_engine(c, 0, &actual) == FVM_OK && actual == FVM_CRASH_ENGINE_NATIVE))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, inner, 0, 0, 0) == FVM_OK && fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    if (argc == 1)
    {
        if (!(std::remove(path) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
