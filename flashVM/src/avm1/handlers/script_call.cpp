/* FlashVM: version-aware script activation preparation for interpreted and generated bodies. */
#include "../encoding_constants.h"
#include "script_call.hpp"
#include "../target_scope.hpp"
#include "support.hpp"
#include "../../core/crash_internal.hpp"
#include "flashvm/standard.h"
#include "native_body.hpp"
#include "../../jit/mode_internal.hpp"
#include "../../standard/function_state.hpp"
#include "../../standard/base_state.hpp"
#include "../../standard/super.hpp"
#include "../../core/property_key.hpp"
#include "../../core/string_internal.hpp"
#include <new>
#include <cstring>

/* A call frame owns parameter bindings and its receiver; script depth is restored
 * even when the interpreter reports an error. */


/* Call script. */
FVM_Result fvm_avm1_call_script(FVM_Context *c,
                                const fvm_detail::string &name,
                                const FVM_ScriptFunction &fn,
                                unsigned int argc,
                                const FVM_Value *args,
                                FVM_Object *receiver,
                                FVM_Value *result,
                                FVM_Object *closure,
                                FVM_Object *callee,
                                FVM_Object *base_clip)
{
    if (c->script_depth >= FVM_AVM1_MAX_SCRIPT_DEPTH)
    {
        return FVM_ERROR;
    }

    /* Initialization / Инициализация локального состояния. */

    /* [GC:] A flagged loaded Function2 inherits the caller cell; its register
     * preload still receives the actual receiver according to the flags. */
    const bool actual_undefined = c->swf_loaded && c->calling_function == callee && c->calling_this_undefined;
    FVM_Value local_this_value;
    fvm_value_undefined(&local_this_value);
    if (!actual_undefined)
    {
        local_this_value.type = FVM_VALUE_OBJECT;
        local_this_value.object = receiver ? receiver : c->global;
    }
    if (c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && fn.function2 &&
        (fn.flags & FVM_AVM1_F2_THIS_FLAGS))
    {
        if (c->script_this_cell)
        {
            local_this_value = c->script_this_cell->value;
        }
        else if (c->script_this_undefined)
        {
            fvm_value_undefined(&local_this_value);
        }
        else
        {
            local_this_value.type = FVM_VALUE_OBJECT;
            local_this_value.object = c->script_this ? c->script_this : c->global;
        }
    }
    FVM_Object *local_this = local_this_value.type == FVM_VALUE_OBJECT ? local_this_value.object : 0;


    /* [GC:] Root the definition timeline and restore the caller's target metadata
     * across both activation preparation and body execution. */
    FVM_AVM1TargetScope targets(c, base_clip ? base_clip : c->avm1_target_active ? 0 : receiver, false);
    FVM_ScriptBodyEntry native_entry;
    if (callee)
    {
        native_entry = callee->script_body;
    }
    FVM_Object *arguments = 0;
    FVM_Value implicit_super;
    fvm_value_undefined(&implicit_super);
    Execution execution;
    execution.target_initialized = true;
    execution.global_registers = c->swf_loaded && (!fn.function2 || !fn.register_count);
    try
    {
        execution.constants = fn.constants;
        /* Allocate the declared loaded span and preserve the unloaded compatibility span. */
        size_t register_slots = fn.function2 ? fn.register_count : FVM_AVM1_LEGACY_REGISTER_COUNT;
        if (fn.function2 && !c->swf_loaded)
        {
            register_slots += FVM_AVM1_HOST_REGISTER_EXTRA_SLOT;
        }
        execution.registers.resize(register_slots);
        for (size_t i = 0; i < execution.registers.size(); ++i)
        {
            fvm_value_undefined(&execution.registers[i]);
        }
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_Object *activation = 0;
    FVM_Result activation_result = fvm_object_create(c, &activation);
    if (activation_result != FVM_OK)
    {
        return activation_result;
    }
    fvm_detail::map<fvm_detail::string, FVM_Value> &frame = activation->properties;

    /* [GC:] The activation retains the function's captured lexical environment. */
    activation->prototype = closure;
    fvm_detail::vector<FVM_Value> constructing;


    /* [GC:] Keep activation, arguments, receiver and registers alive during call setup. */
    struct ConstructionRoots
    {
        FVM_Context *c;
        /* [GC:] Trace construction values separately from private registers. */
        FVM_GCRootFrame values, registers;

        /* [GC:] Push value and register frames before any preload lookup can invoke code. */
        ConstructionRoots(FVM_Context *context,
                          FVM_Object *activation,
                          fvm_detail::vector<FVM_Value> *roots,
                          fvm_detail::vector<FVM_Value> *regs)
            : c(context)
        { /* [GC:] Save the caller root chain before attaching this activation. */
            values.previous = c->gc_frames;
            /* [GC:] Trace temporary receiver, callee and preload values. */
            values.stack = roots;
            /* [GC:] Trace parameter bindings retained by the activation. */
            values.locals = &activation->properties;
            /* [GC:] Retain the activation and its captured lexical scope. */
            values.receiver = activation;
            /* [GC:] Chain the private-register frame after the construction values. */
            registers.previous = &values;
            /* [GC:] Expose live private register contents to the collector. */
            registers.stack = regs;
            /* [GC:] Avoid tracing the same activation properties twice. */
            registers.locals = 0;
            /* [GC:] The preceding frame already retains the activation receiver. */
            registers.receiver = 0;
            /* [GC:] Publish both setup frames as live roots. */
            c->gc_frames = &registers;
        }

        /* [GC:] Restore the previous root chain on normal and exceptional exits. */
        ~ConstructionRoots()
        { /* [GC:] Remove both temporary frames on scope exit. */
            c->gc_frames = values.previous;
        }
    } construction(c, activation, &constructing, &execution.registers);

    try
    {

        FVM_Value capture;
        fvm_value_undefined(&capture);
        capture.type = FVM_VALUE_OBJECT;
        if (receiver)
        {
            capture.object = receiver;
            /* [GC:] Retain this value while subsequent call setup may execute script code. */
            constructing.push_back(capture);
        }
        if (callee)
        {
            capture.object = callee;
            /* [GC:] Retain this value while subsequent call setup may execute script code. */
            constructing.push_back(capture);
        }
        if (native_entry.capture)
        {
            capture.object = native_entry.capture;
            /* [GC:] Retain the entry snapshot before any setup callback can unbind it. */
            constructing.push_back(capture);
        }
        if (!fn.function2 || !(fn.flags & FVM_AVM1_F2_SUPPRESS_SUPER) || (fn.flags & FVM_AVM1_F2_PRELOAD_SUPER))
        {
            FVM_Result made = fvm_super_create(c, callee, receiver, &implicit_super);
            if (made != FVM_OK)
            {
                return made;
            }
            /* [GC:] Retain the facade while the rest of the activation is constructed. */
            constructing.push_back(implicit_super);
        }
        FVM_Result stored =
            c->swf_loaded ? fvm_array_create_like(c, 0, argc, &arguments, true) : fvm_object_create(c, &arguments);
        if (stored != FVM_OK)
        {
            return stored;
        }
        if (!c->swf_loaded)
        {
            arguments->resource_kind = FVM_RESOURCE_OBJECT;
        }
        capture.object = arguments;
        /* [GC:] Retain this value while subsequent call setup may execute script code. */
        constructing.push_back(capture);
        FVM_Value value;
        if (!c->swf_loaded)
        {
            fvm_value_number(&value, argc);
            stored = fvm_object_set(c, arguments, "length", &value);
            if (stored != FVM_OK)
            {
                return stored;
            }
            arguments->attributes["length"] = 1;
        }
        for (unsigned int i = 0; i < argc; ++i)
        {
            if (c->swf_loaded)
            { /* Initial elements are raw own copies; inherited setters must not run. */
                /* [GC:] Own string bytes before publishing the retained element. */
                FVM_Value element = args[i];
                if (element.type == FVM_VALUE_STRING)
                {
                    stored = fvm_value_to_string(c, args + i, &element);
                    if (stored != FVM_OK)
                    {
                        return stored;
                    }
                    element.metatable = args[i].metatable;
                }
                arguments->indexed[i] = element;
            }
            else
            {
                stored = fvm_table_set_index(c, arguments, i, args + i);
                if (stored != FVM_OK)
                {
                    return stored;
                }
            }
        }
        if (callee)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = callee;
            stored = fvm_object_set(c, arguments, "callee", &value);
            if (stored != FVM_OK)
            {
                return stored;
            }
            arguments->attributes["callee"] = 1;
        }
        /* [GC:] The arguments graph retains the previous script function identity,
         * including after its caller frame has returned or been collected. */
        fvm_value_undefined(&value);
        value.type = c->script_callee ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
        value.object = c->script_callee;
        stored = fvm_object_set(c, arguments, "caller", &value);
        if (stored != FVM_OK)
        {
            return stored;
        }
        arguments->attributes["caller"] = 1;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = arguments;

        FVM_Value implicit_arguments = value;
        if (fn.function2 && (fn.flags & FVM_AVM1_F2_SUPPRESS_ARGUMENTS))
        {
            fvm_value_undefined(&value);
        }
        /* Loaded preload/suppression leaves the named lookup in its enclosing scope. */
        if (!c->swf_loaded || !fn.function2 || !(fn.flags & FVM_AVM1_F2_ARGUMENTS_FLAGS))
        {
            stored = fvm_object_set(c, activation, "arguments", &value);
            if (stored != FVM_OK)
            {
                return stored;
            }
        }
        /* Explicit parameters can shadow the implicit arguments binding. */

        {
            FVM_Value named_this;
            fvm_value_undefined(&named_this);
            if (!actual_undefined && (!fn.function2 || !(fn.flags & FVM_AVM1_F2_SUPPRESS_THIS)))
            {
                named_this.type = FVM_VALUE_OBJECT;
                named_this.object = receiver ? receiver : c->global;
            }
            /* Loaded SWF5+ resolves this through its activation cell, not a synthetic
             * ordinary property which would incorrectly fold THIS in SWF5 locals. */
            if (!c->swf_loaded || c->swf_info.version < FVM_SWF_VERSION_5)
            {
                stored = fvm_object_set(c, activation, "this", &named_this);
                if (stored != FVM_OK)
                {
                    return stored;
                }
            }
            FVM_Value named_super = implicit_super;
            if (fn.function2 && (fn.flags & FVM_AVM1_F2_SUPPRESS_SUPER))
            {
                fvm_value_undefined(&named_super);
            }
            if (!c->swf_loaded || !fn.function2 || !(fn.flags & FVM_AVM1_F2_SUPER_FLAGS))
            {
                stored = fvm_object_set(c, activation, "super", &named_super);
                if (stored != FVM_OK)
                {
                    return stored;
                }
            }
        }
        for (size_t i = 0; i < fn.parameters.size(); ++i)
        {
            FVM_Value value;
            fvm_value_undefined(&value);
            if (i < argc)
            {
                value = args[i];
            }

            if (fn.function2 && fn.parameter_registers[i])
            {
                if (fn.parameter_registers[i] < execution.registers.size())
                {
                    execution.registers[fn.parameter_registers[i]] = value;
                }
                continue;
            }
            FVM_Result stored =
                fvm_object_set_bytes(c, activation, fn.parameters[i].data(), fn.parameters[i].size(), &value);
            if (stored != FVM_OK)
            {
                return stored;
            }
        }
        if (fn.function2)
        {
            FVM_Value common[FVM_AVM1_PRELOAD_COUNT];
            for (unsigned int i = 0; i < FVM_AVM1_PRELOAD_COUNT; ++i)
            {
                fvm_value_undefined(common + i);
            }
            if (!actual_undefined)
            {
                common[FVM_AVM1_PRELOAD_THIS_INDEX].type = FVM_VALUE_OBJECT;
                common[FVM_AVM1_PRELOAD_THIS_INDEX].object = receiver ? receiver : c->global;
            }
            common[FVM_AVM1_PRELOAD_ARGUMENTS_INDEX] = implicit_arguments;
            common[FVM_AVM1_PRELOAD_SUPER_INDEX] = implicit_super;
            /* Paired flags suppress preloaded this/super, but arguments still
             * preloads its object even when its named binding is suppressed. */
            if (fn.flags & FVM_AVM1_F2_SUPPRESS_THIS)
            {
                fvm_value_undefined(common);
            }
            if (fn.flags & FVM_AVM1_F2_SUPPRESS_SUPER)
            {
                fvm_value_undefined(common + FVM_AVM1_PRELOAD_SUPER_INDEX);
            }
            /* Match the activation's base-clip selection before its bytecode starts.
             * Display preloads bypass mutable _root/_parent script properties. */
            FVM_Object *preload_clip = base_clip ? base_clip
                                       : receiver && receiver->resource_kind == FVM_RESOURCE_MOVIECLIP
                                           ? receiver
                                           : c->avm1_base_clip;
            if ((fn.flags & FVM_AVM1_F2_DISPLAY_FLAGS) && !preload_clip)
            {
                fvm_detail::map<fvm_detail::string, FVM_Value>::iterator root = c->global->properties.find("_root");
                if (root != c->global->properties.end() && root->second.type == FVM_VALUE_OBJECT &&
                    root->second.object && root->second.object->resource_kind == FVM_RESOURCE_MOVIECLIP)
                {
                    preload_clip = root->second.object;
                }
            }
            if ((preload_clip || receiver) && (fn.flags & FVM_AVM1_F2_PRELOAD_ROOT))
            {
                stored =
                    preload_clip
                        ? fvm_movieclip_preload_display(c, preload_clip, false, common + FVM_AVM1_PRELOAD_ROOT_INDEX)
                        : fvm_object_get(c, receiver, "_root", common + FVM_AVM1_PRELOAD_ROOT_INDEX);
                if (stored != FVM_OK)
                {
                    return stored;
                }
                /* [GC:] Retain this value while subsequent call setup may execute script code. */
                constructing.push_back(common[FVM_AVM1_PRELOAD_ROOT_INDEX]);
            }
            if ((fn.flags & FVM_AVM1_F2_PRELOAD_ROOT) &&
                common[FVM_AVM1_PRELOAD_ROOT_INDEX].type == FVM_VALUE_UNDEFINED)
            {
                stored = fvm_get_global(c, "_root", common + FVM_AVM1_PRELOAD_ROOT_INDEX);
                if (stored != FVM_OK)
                {
                    return stored;
                }
            }
            if ((preload_clip || receiver) && (fn.flags & FVM_AVM1_F2_PRELOAD_PARENT))
            {
                stored =
                    preload_clip
                        ? fvm_movieclip_preload_display(c, preload_clip, true, common + FVM_AVM1_PRELOAD_PARENT_INDEX)
                        : fvm_object_get(c, receiver, "_parent", common + FVM_AVM1_PRELOAD_PARENT_INDEX);
                if (stored != FVM_OK)
                {
                    return stored;
                }
            }
            common[FVM_AVM1_PRELOAD_GLOBAL_INDEX].type = FVM_VALUE_OBJECT;
            common[FVM_AVM1_PRELOAD_GLOBAL_INDEX].object = c->global;
            const unsigned int bits[] = {FVM_AVM1_F2_PRELOAD_THIS,
                                         FVM_AVM1_F2_PRELOAD_ARGUMENTS,
                                         FVM_AVM1_F2_PRELOAD_SUPER,
                                         FVM_AVM1_F2_PRELOAD_ROOT,
                                         FVM_AVM1_F2_PRELOAD_PARENT,
                                         FVM_AVM1_F2_PRELOAD_GLOBAL};
            unsigned int slot = 1;
            for (unsigned int i = 0; i < FVM_AVM1_PRELOAD_COUNT; ++i)
            {
                if (fn.flags & bits[i])
                { /* A parentless timeline leaves this preload slot available to _global. */
                    if (i == FVM_AVM1_PRELOAD_PARENT_INDEX &&
                        (common[i].type == FVM_VALUE_UNDEFINED || common[i].type == FVM_VALUE_NULL))
                    {
                        continue;
                    }
                    if (slot < execution.registers.size())
                    {
                        execution.registers[slot] = common[i];
                    }
                    ++slot;
                }
            }
        }
        fvm_detail::map<fvm_detail::string, bool> seen;
        /* Loaded Flash arguments are value copies; host calls keep their mapped bindings. */
        for (size_t i = fn.parameters.size(); !c->swf_loaded && i; --i)
        {
            size_t index = i - 1;
            if (fn.function2 && fn.parameter_registers[index])
            {
                continue;
            }
            fvm_detail::string name;
            stored = fvm_property_key(fn.parameters[index].data(), fn.parameters[index].size(), name);
            if (stored != FVM_OK)
            {
                return stored;
            }
            if (seen.find(name) != seen.end())
            {
                continue;
            }
            seen[name] = true;
            if (index >= argc)
            {
                continue;
            }
            stored = fvm_function_map_argument(c, arguments, (unsigned long)index, activation, name);
            if (stored != FVM_OK)
            {
                return stored;
            }
        }
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_ScriptFunction snapshot;
    try
    {
        snapshot = fn;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }


    /* Define DebugFrame struct and its representation. */
    struct DebugFrame
    {
        FVM_Context *context;
        fvm_detail::string function, source;
        unsigned long line, offset;
        unsigned int depth;


        /* Initialize Debug Frame and its owned state. */
        explicit DebugFrame(FVM_Context *c)
            : context(c), function(c->debug_function), source(c->debug_source), line(c->debug_line),
              offset(c->debug_offset), depth(c->script_depth)
        {
        }


        /* Release the resources owned by this DebugFrame instance. */
        ~DebugFrame()
        {

            /* Computation / Вычисление и обновление состояния. */
            context->debug_function.swap(function);
            context->debug_source.swap(source);
            context->debug_line = line;
            context->debug_offset = offset;
            context->script_depth = depth;
        }
    };

    FVM_Result status;
    bool ran_jit = false;
    unsigned long native_fault_offset = 0;
    try
    {


        DebugFrame debug(c);
        c->debug_function = name;
        c->debug_line = 0;


        /* [GC:] Restore depth and caller identity after execution or preparation OOM. */
        struct DepthScope
        {
            FVM_Context *context;        /* [GC:] Context whose active frame is scoped. */
            unsigned int previous;       /* Suspended script recursion depth. */
            FVM_Object *previous_callee; /* [GC:] Caller retained by its suspended root frame. */


            /* [GC:] Enter one script activation after preserving the caller's exact depth. */
            DepthScope(FVM_Context *c, FVM_Object *callee)
                : context(c), previous(c->script_depth), previous_callee(c->script_callee)
            {
                ++context->script_depth;
                context->script_callee = callee;
            }


            /* [GC:] Restore depth even if lexical-scope registration throws before bytecode entry.
             */
            ~DepthScope()
            {
                context->script_depth = previous;
                context->script_callee = previous_callee;
            }
        } depth(c, callee);

        /* [GC:] The activation retains the function's captured lexical environment. */
        activation->prototype = closure;
        c->lexical_scopes.push_back(activation);


        /* Define PopScope struct and its representation. */
        struct PopScope
        {
            FVM_Context *c;


            /* Pop Scope. */
            explicit PopScope(FVM_Context *context) : c(context)
            {
            }


            /* Release the resources owned by this PopScope instance. */
            ~PopScope()
            {
                c->lexical_scopes.pop_back();
            }
        } scope(c);

        FVM_ExecutionMode mode = fvm_debug_has_breakpoints(c, c->debug_function) ? FVM_EXECUTION_INTERPRETER
                                                                                 : fvm_execution_effective_mode(c);


        /* Borrow existing activation/argument storage; report copies are made only on failure. */
        FVM_CrashScope diagnostic(c, &name, snapshot.body.begin(), snapshot.body.size(), argc, args, activation);
        if (c->crash.active && !c->crash.busy && !c->execution_mode.replay_depth)
        {
            c->crash.active->function = &snapshot;
            c->crash.active->registers =
                execution.global_registers ? c->avm1_global_registers : execution.registers.begin();
            c->crash.active->register_count =
                execution.global_registers ? FVM_EXEC_GLOBAL_REGISTER_COUNT : (unsigned int)execution.registers.size();
        }
        if (mode == FVM_EXECUTION_INTERPRETER && native_entry.callback == fvm_jit_body)
        {
            native_entry = FVM_ScriptBodyEntry();
        }
        if (mode == FVM_EXECUTION_JIT && !native_entry.callback)
        {
            FVM_JitCode *code = 0;
            status = fvm_execution_select_code(c, snapshot.body.begin(), snapshot.body.size(), &code);
            if (status != FVM_OK)
            {
                return status;
            }
            if (code)
            {
                native_entry.callback = fvm_jit_body;
                native_entry.userdata = code;
            }
        }
        if (c->crash.active && !c->crash.busy && !c->execution_mode.replay_depth)
        {
            c->crash.active->external = native_entry.callback && native_entry.callback != fvm_jit_body;
        }
        if (native_entry.callback)
        {
            ran_jit = native_entry.callback == fvm_jit_body;
            FVM_ScriptFrame prepared;
            prepared.struct_size = sizeof(prepared);
            prepared.abi_version = FVM_SCRIPT_FRAME_ABI;
            prepared.function = callee;
            prepared.receiver = receiver ? receiver : c->global;
            prepared.implicit_this = local_this;
            prepared.activation = activation;
            prepared.arguments = arguments;
            prepared.super_value = implicit_super;
            if (fn.function2 && (fn.flags & FVM_AVM1_F2_SUPPRESS_SUPER))
            {
                fvm_value_undefined(&prepared.super_value);
            }
            prepared.argc = argc;
            prepared.argv = args;
            prepared.register_count =
                execution.global_registers ? FVM_EXEC_GLOBAL_REGISTER_COUNT : (unsigned int)execution.registers.size();
            prepared.registers = execution.global_registers ? c->avm1_global_registers : execution.registers.begin();
            prepared.capture = native_entry.capture;
            status = fvm_exec_native_body(c, native_entry, prepared, result, &execution.constants, &local_this_value);
            native_fault_offset = c->debug_offset;
        }
        else
        {
            status = fvm_avm1_interpret_function(c, snapshot, result, &frame, receiver, &execution, &local_this_value);
        }
        fvm_crash_failure(c, status);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (ran_jit && status == FVM_EXCEPTION)
    {
        fvm_execution_replay_function(
            c, name, snapshot, argc, args, receiver, closure, callee, base_clip, native_fault_offset);
    }
    /* A returned bytecode literal must outlive the function snapshot. */
    if (status == FVM_OK && result && result->type == FVM_VALUE_STRING)
    {
        size_t len = fvm_string_size(*result);
        char *copy = new (std::nothrow) char[len + 1];
        if (!copy)
        {
            return FVM_OUT_OF_MEMORY;
        }
        std::memcpy(copy, result->string, len);
        copy[len] = 0;
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(c, copy, len);
        }
        catch (...)
        {
            delete[] copy;
            return FVM_OUT_OF_MEMORY;
        }
        result->string = copy;
    }
    return status;
}


/* Invoke avm1 script. */
FVM_Result fvm_avm1_invoke_script(FVM_Context *c,
                                  const FVM_ScriptFunction &body,
                                  FVM_Object *closure,
                                  FVM_Object *receiver,
                                  unsigned int argc,
                                  const FVM_Value *args,
                                  FVM_Value *out,
                                  FVM_Object *callee,
                                  FVM_Object *base_clip)
{
    return fvm_avm1_call_script(c,
                                body.name.empty() ? fvm_detail::string("<function>") : body.name,
                                body,
                                argc,
                                args,
                                receiver ? receiver : c->global,
                                out,
                                closure,
                                callee,
                                base_clip);
}
