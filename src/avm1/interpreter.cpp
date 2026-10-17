#include "../core/crash_internal.hpp"
/* FlashVM: AVM1 bytecode decoding and execution: interpreter.
 * File: src/avm1/interpreter.cpp.
 */

#include "../core/this_cell.hpp"
#include "function_record.hpp"
#include "function_definition.hpp"
#include "with_record.hpp"
#include "action_codes.h"
#include "encoding_constants.h"
#include "operand_constants.h"
#include "operand_stack.hpp"
#include "number_decode.hpp"
#include "literal_constants.h"
#include "../core/numeric_constants.h"
#include "../swf/record_constants.h"
#include "handlers/script_call.hpp"
#include "../jit/mode_internal.hpp"
#include "block.hpp"
#include "coercion.hpp"
#include "target_scope.hpp"
#include "lexical_scope.hpp"
#include "../standard/function_state.hpp"
#include "../standard/inheritance.hpp"
#include "../standard/super.hpp"
#include "../standard/base_state.hpp"
#include "../standard/primitive_state.hpp"
#include "../standard/movieclip_state.hpp"
#include "../standard/movieclip_scene.hpp"
#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include "try_record.hpp"
#include "try_constants.h"
#include "../core/exception_internal.hpp"
#include "../core/runtime.hpp"
#include "../core/property_key.hpp"
#include "../core/native_call_guard.hpp"
/* [GC:] Commit finalizer entry only after VM preparation completes. */
#include "../core/finalizer_entry.hpp"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/worker.h"
#include "../details/vector.hpp"
#include "../details/string.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <cmath>
#include <limits>

namespace
{
/* Define Action struct and its representation. */
struct Action
{
    unsigned char op;
    size_t offset, next, payload, length;
};

/* Compute and return u16. */
static unsigned int u16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << FVM_AVM1_BYTE_BITS);
}

/* Compute and return u32. */
static unsigned long u32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << FVM_AVM1_BYTE_BITS) | ((unsigned long)p[2] << (2 * FVM_AVM1_BYTE_BITS)) |
           ((unsigned long)p[3] << (3 * FVM_AVM1_BYTE_BITS));
}
/* Use numeric string truth in loaded SWFs below version 7, preserving conversion failures. */
static FVM_Result action_truth(FVM_Context *context, const FVM_Value &value, bool &out)
{
    FVM_Value converted;
    FVM_Result status = fvm_exec_to_boolean(context, &value, &converted);
    if (status == FVM_OK)
        out = converted.boolean != 0;
    return status;
}

/* Compute and return own string. */
static FVM_Result own_string(FVM_Context *context, const char *text, FVM_Value *value,
                             size_t length = (size_t)-1)
{
    if (length == (size_t)-1)
        length = std::strlen(text);
    char *copy = new (std::nothrow) char[length + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;

    /* Computation / Вычисление и обновление состояния. */
    std::memcpy(copy, text, length);
    copy[length] = 0;
    try
    {
        /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
         * caller cleanup active. */
        fvm_context_adopt_string(context, copy, length);
    }
    catch (...)
    {
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    return fvm_value_string_bytes(value, copy, length);
}

/* Select variable/frame-call scope, using the display root after an invalid target selection. */
static FVM_Result variable_receiver(FVM_Context *c, FVM_Object *self, FVM_Object **out)
{
    *out = self;
    if (!c->avm1_target_active || !c->avm1_base_clip)
        return FVM_OK;
    if (c->avm1_target_clip)
    {
        *out = c->avm1_target_clip;
        return FVM_OK;
    }
    FVM_Value root_path;
    FVM_Result status = fvm_value_string(&root_path, "/");
    return status == FVM_OK ? fvm_movieclip_resolve_path(c, c->avm1_base_clip, root_path, out)
                            : status;
}

/* Resolve ordinary bindings through the C layer while choosing the caller's default receiver. */
static FVM_Result lookup_variable(FVM_Context *c,
                                  fvm_detail::map<fvm_detail::string, FVM_Value> *locals,
                                  FVM_Object *self, const fvm_detail::string &name, FVM_Value *out,
                                  bool *defined = 0, FVM_Object **call_receiver = 0)
{
    FVM_Object *target = 0, *default_receiver = c->global;
    FVM_Result status = variable_receiver(c, self, &target);
    if (status == FVM_OK && c->swf_loaded && c->avm1_base_clip)
        status = variable_receiver(c, c->avm1_base_clip, &default_receiver);
    if (status != FVM_OK)
        return status;
    FVM_Value key;
    status = fvm_value_string_bytes(&key, name.data(), name.size());
    FVM_Object *selected_receiver = 0;
    int found = 0;
    if (status == FVM_OK)
        status = fvm_exec_variable_lookup(c, c->lexical_scopes.empty()
            ? 0 : c->lexical_scopes.back(), target,
            c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                ? c->script_this : self,
            default_receiver, locals != 0, &key, out, &selected_receiver, &found);
    if (status == FVM_OK)
    {
        if (defined)
            *defined = found != 0;
        if (call_receiver)
            *call_receiver = selected_receiver;
    }
    return status;
}

/* Find the managed activation corresponding to the interpreter's existing frame root. */
static FVM_Object *local_activation(FVM_Context *c,
                                     fvm_detail::map<fvm_detail::string, FVM_Value> *locals)
{
    if (locals)
        for (size_t at = c->lexical_scopes.size(); at; --at)
        {
            FVM_Object *scope = c->lexical_scopes[at - 1];
            if (scope && &scope->properties == locals)
                return scope;
        }
    return 0;
}

/* Resolve qualified paths through the shared C service, then ordinary counted bindings. */
static FVM_Result lookup_variable_value(FVM_Context *c,
                                        fvm_detail::map<fvm_detail::string, FVM_Value> *locals,
                                        FVM_Object *self, const FVM_Value &value, FVM_Value *out,
                                        bool *defined = 0)
{
    FVM_Object *target = 0;
    FVM_Result result = variable_receiver(c, self, &target);
    int handled = 0, present = 0;
    if (result == FVM_OK)
        result = fvm_exec_variable_path(c, c->lexical_scopes.empty()
            ? 0 : c->lexical_scopes.back(), target, c->script_this ? c->script_this : self,
            &value, out, &handled, &present);
    if (result != FVM_OK || handled)
    {
        if (result == FVM_OK && defined)
            *defined = present != 0;
        return result;
    }
    fvm_detail::string key;
    result = fvm_property_key(value.string, fvm_string_size(value), key);
    return result == FVM_OK ? lookup_variable(c, locals, self, key, out, defined) : result;
}

/* Execute the current value. */
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared = 0, size_t base = 0, unsigned int depth = 0,
                          bool bounded = false, const FVM_Value *implicit_this = 0);
/* Execute the current value. */
/* Keep root source metadata alive through fault capture; functions own their shared scope. */
static FVM_Result execute_impl(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared, size_t base, unsigned int depth, bool bounded,
                          const FVM_Value *implicit_this)
;
/* Capture errors before caller activations and their borrowed source/arguments are released. */
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared, size_t base, unsigned int depth, bool bounded,
                          const FVM_Value *implicit_this)
{
    FVM_CrashScope diagnostic(c, 0, data, size, 0, 0, 0, !shared);
    FVM_Result status = execute_impl(c, data, size, returned, locals, self, inherited,
                                     shared, base, depth, bounded, implicit_this);
    fvm_crash_failure(c, status);
    return status;
}
/* Execute one AVM1 block with the existing interpreter state and root policy. */
static FVM_Result execute_impl(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared, size_t base, unsigned int depth, bool bounded,
                          const FVM_Value *implicit_this)
{

    /* Initialization / Инициализация локального состояния. */
    /* Nested functions and native reentry consume the same outer instruction
     * budget. Separate operand stacks must not reset this security boundary. */
    /* Define BudgetGuard struct and its representation. */
    struct BudgetGuard
    {
        FVM_Context *context;
        /* Initialize Budget Guard and its owned state. */
        explicit BudgetGuard(FVM_Context *c) : context(c)
        {
            if (!c->avm1_execution_depth)
                c->avm1_steps = 0;
            ++c->avm1_execution_depth;
        }
        /* Release the resources owned by this BudgetGuard instance. */
        ~BudgetGuard()
        {
            --context->avm1_execution_depth;
        }
    } budget(c);
    Execution owned;
    Execution &execution = shared ? *shared : owned;
    fvm_detail::vector<fvm_detail::string> &constants = execution.constants;
    fvm_detail::vector<FVM_Value> &registers = execution.registers;
    fvm_detail::vector<FVM_Value> &stack = execution.stack;
    fvm_detail::vector<Action> actions;
    /* Register live operand stack, lexical bindings and receiver as GC roots.
       The frame is popped on every return path, including exceptions. */
    /* Define RootGuard struct and its representation. */
    /* [GC:] Объявляем представление состояния сборщика или корней. */
    struct RootGuard
    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
    {
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Context *context;
        /* [GC:] Описываем или используем состояние сборщика и его активные корни. */
        FVM_GCRootFrame frame, register_frame;
        FVM_ThisCellScope this_scope; /* [GC:] Typed activation and suspended caller values. */
        /* Initialize Root Guard and its owned state. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        RootGuard(FVM_Context *c, fvm_detail::vector<FVM_Value> *s,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  fvm_detail::map<fvm_detail::string, FVM_Value> *l, FVM_Object *r,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  fvm_detail::vector<FVM_Value> *reg, const FVM_Value *current_this = 0, bool new_cell = false)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            : context(c), this_scope(c, fvm_this_cell_initial(c, current_this,
                                                        c->script_this ? c->script_this : r), new_cell)
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {

            /* Computation / Вычисление и обновление состояния. */
            /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
            frame.previous = c->gc_frames;
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            frame.stack = s;
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            frame.locals = l;
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            frame.receiver = r;
            /* [GC:] Сохраняем предыдущий корень для восстановления цепочки после выхода. */
            register_frame.previous = &frame;
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            register_frame.stack = reg;
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            register_frame.locals = 0;
            /* [GC:] Подключаем стек, локальные значения или получателя к активному корню вызова. */
            /* [GC:] The register frame also retains the distinct implicit receiver. */
            register_frame.receiver = c->script_this;
            /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO. */
            c->gc_frames = &register_frame;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* Release the resources owned by this RootGuard instance. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        ~RootGuard()
        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        {
            /* [GC:] Обновляем цепочку активных корней; порядок восстановления должен быть LIFO. */
            context->gc_frames = frame.previous;
            /* [GC:] Restore caller identity on normal, exception and OOM exits. */
            /* The typed cell scope restores the caller after detaching these frames. */
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    } roots(c, &stack, locals, self, &registers, implicit_this, !depth || implicit_this != 0);
    size_t pos = 0, pc = 0, i;
    unsigned long &steps = execution.steps;
    if (!c || (!data && size) || depth > FVM_AVM1_MAX_BLOCK_DEPTH)
        return FVM_INVALID_ARGUMENT;
    ++c->execution_mode.stats.interpreter_entries;
    if (returned && !shared)
        fvm_value_undefined(returned);
    try
    {
        /* [GC:] Block activations inherit targets; fresh script/frame activations reset them. */
        FVM_AVM1TargetScope target_scope(c, self, shared && execution.target_initialized);
        execution.target_initialized = true;
        if (!shared)
        {
            if (inherited)
                constants = *inherited;
            execution.global_registers = c->swf_loaded;
            registers.resize(FVM_AVM1_LEGACY_REGISTER_COUNT);
            for (size_t reg = 0; reg < registers.size(); ++reg)
                fvm_value_undefined(&registers[reg]);
        }
        while (pos < size)
        {
            Action a;
            a.offset = pos;
            a.op = data[pos++];
            a.payload = pos;
            a.length = 0;
            if (a.op >= FVM_AVM1_LONG_ACTION_BASE)
            {
                if (size - pos < 2)
                    return FVM_INVALID_ARGUMENT;
                a.length = u16(data + pos);
                pos += 2;
                a.payload = pos;
                if (a.length > size - pos)
                    return FVM_INVALID_ARGUMENT;
                pos += a.length;
                if (a.op == FVM_AVM1_ACTION_DEFINE_FUNCTION ||
                    a.op == FVM_AVM1_ACTION_DEFINE_FUNCTION2)
                {
                    FVM_FunctionRecord record;
                    if (!fvm_function_record(data, size, a.payload, a.length, record,
                                             a.op == FVM_AVM1_ACTION_DEFINE_FUNCTION2, c->swf_loaded))
                        return FVM_INVALID_ARGUMENT;
                    pos = record.next;
                }
                if (a.op == FVM_AVM1_ACTION_WITH)
                {
                    size_t body = 0, count = 0, next = 0;
                    if (!fvm_with_record(data, size, a.payload, a.length, body, count, next))
                        return FVM_INVALID_ARGUMENT;
                    pos = next;
                }
                if (a.op == FVM_AVM1_ACTION_TRY)
                {
                    FVM_TryRecord record;
                    if (!fvm_try_record(data, size, a.payload, a.length, record))
                        return FVM_INVALID_ARGUMENT;
                    pos += record.sizes[FVM_AVM1_TRY_BODY_INDEX] +
                           record.sizes[FVM_AVM1_TRY_CATCH_INDEX] +
                           record.sizes[FVM_AVM1_TRY_FINALLY_INDEX];
                }
            }
            a.next = pos;
            actions.push_back(a);
            if (a.op == FVM_AVM1_ACTION_END)
                break;
        }
        if (pos != size ||
            (!shared && !bounded && (actions.empty() || actions.back().op != FVM_AVM1_ACTION_END)))
            return FVM_INVALID_ARGUMENT;
        if (bounded && (actions.empty() || actions.back().op != FVM_AVM1_ACTION_END))
        {
            /* Model the exact record boundary as a terminal action, without changing source bytes.
             */
            Action terminal;
            terminal.op = FVM_AVM1_ACTION_END;
            terminal.offset = terminal.next = terminal.payload = size;
            terminal.length = 0;
            actions.push_back(terminal);
        }
        /* [GC:] Action parsing/root-frame preparation can fail without consuming finalization. */
        fvm_finalizer_enter(c);
        while (pc < actions.size())
        {
            const Action &a = actions[pc];
            ++steps;
            /* [GC:] Poll at the established deterministic boundary with all operand roots live. */
            FVM_Result checkpoint = fvm_exec_checkpoint(
                c, 1, (unsigned long)(base + a.offset),
                (steps & FVM_AVM1_POLL_INTERVAL_MASK) == 1UL ? FVM_EXEC_POLL : 0);
            if (checkpoint != FVM_OK)
                return checkpoint;
            FVM_Result breakpoint = fvm_debug_breakpoint_poll(c);
            if (breakpoint != FVM_OK)
                return breakpoint;
            if (a.op == FVM_AVM1_ACTION_END)
                return FVM_OK;
            if (a.op == FVM_AVM1_ACTION_RETURN)
            { /* Return */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                if (returned)
                    *returned = stack.back();
                execution.returned = true;
                return FVM_OK;
            }
            if (a.op == FVM_AVM1_ACTION_THROW)
            { /* Throw: preserve the value while unwinding. */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value value = stack.back();
                FVM_Result result = fvm_exception_raise(c, &value);
                stack.pop_back();
                return result;
            }
            if (a.op == FVM_AVM1_ACTION_WITH)
            {
                size_t body = 0, count = 0, next = 0;
                if (!fvm_with_record(data, size, a.payload, a.length, body, count, next))
                    return FVM_INVALID_ARGUMENT;
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value value = stack.back();
                if (value.type != FVM_VALUE_NULL && value.type != FVM_VALUE_UNDEFINED)
                {
                    FVM_Object *parent = c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
                    FVM_Object *scope = 0;
                    FVM_Result status = fvm_exec_with_scope(c, parent, &value, &scope);
                    if (status != FVM_OK) return status;
                    /* [GC:] Publish before dropping the operand retaining its wrapped object. */
                    FVM_AVM1LexicalScope scope_exit(c);
                    scope_exit.publish(scope);
                    stack.pop_back();
                    /* [GC:] With copies the current this value into its child activation. */
                    FVM_ThisCellScope with_this(c, fvm_this_cell_initial(c, 0, self));
                    status = execute(c, data + body, count, returned, locals, self, 0,
                                     &execution, base + body, depth + 1);
                    if (status != FVM_OK || execution.returned) return status;
                }
                else stack.pop_back();
                ++pc;
                continue;
            }
            if (a.op == FVM_AVM1_ACTION_TRY)
            {
                FVM_TryRecord record;
                if (!fvm_try_record(data, size, a.payload, a.length, record))
                    return FVM_INVALID_ARGUMENT;
                fvm_detail::vector<FVM_Value> baseline = stack;
                RootGuard baseline_roots(c, &baseline, 0, 0, &registers);
                FVM_Result result =
                    execute(c, data + record.body, record.sizes[FVM_AVM1_TRY_BODY_INDEX], returned,
                            locals, self, 0, &execution, base + record.body, depth + 1);
                /* [GC:] Values saved across catch/finally need roots independent of the
                 * operand stack, which is restored during exception unwinding. */
                fvm_detail::vector<FVM_Value> saved;
                saved.resize(FVM_AVM1_TRY_ROOT_COUNT);
                for (size_t n = 0; n < saved.size(); ++n)
                    fvm_value_undefined(&saved[n]);
                RootGuard saved_roots(c, &saved, 0, 0, &registers);
                if (result == FVM_EXCEPTION && (record.flags & FVM_AVM1_TRY_CATCH))
                {
                    /* [GC:] Catch owns a copied cell; Try/Finally retain the parent cell. */
                    FVM_ThisCellScope catch_this(c, fvm_this_cell_initial(c, 0, self),
                                                 c->swf_loaded);
                    saved[FVM_AVM1_TRY_THROWN_ROOT] = c->exception_value;
                    stack = baseline;
                    fvm_exception_clear(c);
                    /* [GC:] Unloaded host catches retain their temporary managed scope. */
                    FVM_AVM1LexicalScope catch_scope(c);
                    fvm_detail::map<fvm_detail::string, FVM_Value> *bindings = locals;
                    if (!c->swf_loaded && !bindings)
                    {
                        FVM_Object *activation = 0;
                        FVM_Result created = fvm_object_create(c, &activation);
                        if (created != FVM_OK)
                            return created;
                        activation->prototype = c->lexical_scopes.empty()
                            ? 0 : c->lexical_scopes.back();
                        catch_scope.publish(activation);
                        bindings = &activation->properties;
                    }
                    bool had_binding = false;
                    if (record.flags & FVM_AVM1_TRY_CATCH_REGISTER)
                    {
                        if (execution.global_registers)
                        {
                            FVM_Value ignored;
                            FVM_Result stored = fvm_exec_global_register(
                                c, 1, record.reg, &saved[FVM_AVM1_TRY_THROWN_ROOT], &ignored);
                            if (stored != FVM_OK) return stored;
                        }
                        else if (record.reg < registers.size())
                            registers[record.reg] = saved[FVM_AVM1_TRY_THROWN_ROOT];
                        else if (!c->swf_loaded)
                            return FVM_INVALID_ARGUMENT;
                    }
                    else if (c->swf_loaded)
                    {
                        FVM_Value name;
                        FVM_Result bound = fvm_value_string(&name, record.name);
                        FVM_Object *target = 0;
                        if (bound == FVM_OK)
                            bound = variable_receiver(c, self, &target);
                        if (bound == FVM_OK)
                            bound = fvm_exec_variable_set(c, c->lexical_scopes.empty()
                                ? 0 : c->lexical_scopes.back(),
                                target && target->resource_kind == FVM_RESOURCE_MOVIECLIP
                                    ? target : c->global,
                                c->script_this ? c->script_this : self,
                                &name, &saved[FVM_AVM1_TRY_THROWN_ROOT]);
                        if (bound != FVM_OK)
                            return bound;
                    }
                    else
                    {
                        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator old =
                            bindings->find(record.name);
                        had_binding = old != bindings->end();
                        if (had_binding)
                            saved[FVM_AVM1_TRY_BINDING_ROOT] = old->second;
                        (*bindings)[record.name] = saved[FVM_AVM1_TRY_THROWN_ROOT];
                    }
                    result = execute(c, data + record.body + record.sizes[FVM_AVM1_TRY_BODY_INDEX],
                                     record.sizes[FVM_AVM1_TRY_CATCH_INDEX], returned, bindings,
                                     self, 0, &execution,
                                     base + record.body + record.sizes[FVM_AVM1_TRY_BODY_INDEX],
                                     depth + 1);
                    if (!c->swf_loaded && !(record.flags & FVM_AVM1_TRY_CATCH_REGISTER))
                    {
                        if (had_binding)
                            (*bindings)[record.name] = saved[FVM_AVM1_TRY_BINDING_ROOT];
                        else
                            bindings->erase(record.name);
                    }
                }
                if ((record.flags & FVM_AVM1_TRY_FINALLY) &&
                    (result == FVM_OK || result == FVM_EXCEPTION))
                {
                    bool was_return = execution.returned;
                    bool was_exception = c->exception_pending;
                    bool was_fault = c->fault_pending;
                    FVM_ExceptionInfo fault = c->fault;
                    if (returned)
                        saved[FVM_AVM1_TRY_RETURN_ROOT] = *returned;
                    saved[FVM_AVM1_TRY_PENDING_ROOT] = c->exception_value;
                    execution.returned = false;
                    fvm_exception_clear(c);
                    size_t start = record.body + record.sizes[FVM_AVM1_TRY_BODY_INDEX] +
                                   record.sizes[FVM_AVM1_TRY_CATCH_INDEX];
                    FVM_Result final_result =
                        execute(c, data + start, record.sizes[FVM_AVM1_TRY_FINALLY_INDEX], returned,
                                locals, self, 0, &execution, base + start, depth + 1);
                    if (final_result != FVM_OK || execution.returned)
                        result = final_result;
                    else
                    {
                        execution.returned = was_return;
                        if (returned)
                            *returned = saved[FVM_AVM1_TRY_RETURN_ROOT];
                        c->exception_pending = was_exception;
                        c->exception_value = saved[FVM_AVM1_TRY_PENDING_ROOT];
                        c->fault_pending = was_fault;
                        c->fault = fault;
                    }
                }
                if (result != FVM_OK || execution.returned)
                    return result;
                ++pc;
                continue;
            }
            if (a.op == FVM_AVM1_ACTION_CONSTANT_POOL)
            { /* ConstantPool: replace the current pool atomically. */
                size_t p = a.payload, end = p + a.length;
                if (end - p < 2)
                    return FVM_INVALID_ARGUMENT;
                unsigned int count = u16(data + p);
                p += 2;
                fvm_detail::vector<fvm_detail::string> pool;
                for (unsigned int entry = 0; entry < count; ++entry)
                {
                    size_t start = p;
                    while (p < end && data[p])
                        ++p;
                    if (p == end)
                        return FVM_INVALID_ARGUMENT;
                    pool.push_back(fvm_detail::string((const char *)(data + start), p - start));
                    ++p;
                }
                if (p != end)
                    return FVM_INVALID_ARGUMENT;
                constants.swap(pool);
            }
            else if (a.op == FVM_AVM1_ACTION_STORE_REGISTER)
            { /* StoreRegister leaves the operand on stack. */
                if (a.length != FVM_AVM1_STORE_REGISTER_BYTES)
                    return FVM_INVALID_ARGUMENT;
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                unsigned int reg = data[a.payload];
                if (execution.global_registers)
                {
                    FVM_Value ignored;
                    FVM_Result stored = fvm_exec_global_register(c, 1, reg, &stack.back(), &ignored);
                    if (stored != FVM_OK) return stored;
                }
                else if (reg < registers.size())
                    registers[reg] = stack.back();
                else if (!c->swf_loaded)
                    return FVM_INVALID_ARGUMENT;
            }
            else if (a.op == FVM_AVM1_ACTION_DEFINE_FUNCTION ||
                     a.op == FVM_AVM1_ACTION_DEFINE_FUNCTION2)
            {
                FVM_FunctionRecord record;
                if (!fvm_function_record(data, size, a.payload, a.length, record,
                                         a.op == FVM_AVM1_ACTION_DEFINE_FUNCTION2, c->swf_loaded))
                    return FVM_INVALID_ARGUMENT;
                FVM_ScriptFunction fn;
                FVM_Result prepared = fvm_avm1_prepare_function(data, record, c->swf_loaded,
                                                                 &constants, fn);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Object *object = 0;
                FVM_Result r = fvm_function_make_script(
                    c, fn, c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back(), &object);
                if (r != FVM_OK)
                    return r;
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = object;
                if (record.name_size)
                {
                    const fvm_detail::string &name = fn.name;
                    c->script_functions[name] = fn;
                    FVM_Object *target = 0;
                    r = variable_receiver(c, self, &target);
                    FVM_Value binding_name;
                    if (r == FVM_OK)
                        r = fvm_value_string_bytes(&binding_name, name.data(), name.size());
                    if (r == FVM_OK)
                        r = fvm_exec_variable_declare(c, c->lexical_scopes.empty()
                            ? 0 : c->lexical_scopes.back(), local_activation(c, locals),
                            target && target->resource_kind == FVM_RESOURCE_MOVIECLIP
                                ? target : c->global,
                            c->script_this ? c->script_this : self,
                            FVM_EXEC_DECLARE_INITIALIZED, &binding_name, &value);
                    if (r != FVM_OK)
                        return r;
                }
                else
                    stack.push_back(value);
            }
            else if (a.op == FVM_AVM1_ACTION_PUSH)
            { /* Push: strings, null, undefined, boolean, double, integer */
                size_t p = a.payload, end = p + a.length;
                while (p < end)
                {
                    FVM_Value v;
                    fvm_value_undefined(&v);
                    unsigned int type = data[p++];
                    if (type == FVM_AVM1_PUSH_STRING)
                    {
                        size_t start = p;
                        while (p < end && data[p])
                            ++p;
                        if (p == end)
                            return FVM_INVALID_ARGUMENT;
                        v.type = FVM_VALUE_STRING;
                        FVM_Result result = own_string(c, (const char *)(data + start), &v);
                        if (result != FVM_OK)
                            return result;
                        ++p;
                    }
                    else if (type == FVM_AVM1_PUSH_FLOAT)
                    {
                        if (end - p < FVM_AVM1_DWORD_BYTES)
                            return FVM_INVALID_ARGUMENT;
                        unsigned long bits = u32(data + p);
                        fvm_value_number(&v, fvm_avm1_decode_float(bits));
                        p += FVM_AVM1_DWORD_BYTES;
                    }
                    else if (type == FVM_AVM1_PUSH_REGISTER)
                    {
                        if (end - p < 1)
                            return FVM_INVALID_ARGUMENT;
                        unsigned int reg = data[p++];
                        if (execution.global_registers)
                        {
                            FVM_Result read = fvm_exec_global_register(c, 0, reg, 0, &v);
                            if (read != FVM_OK) return read;
                        }
                        else if (reg < registers.size())
                            v = registers[reg];
                        else if (!c->swf_loaded)
                            return FVM_INVALID_ARGUMENT;
                    }
                    else if (type == FVM_AVM1_PUSH_DOUBLE)
                    {
                        if (end - p < FVM_AVM1_DOUBLE_BYTES)
                            return FVM_INVALID_ARGUMENT;
                        /* AVM1 stores the high 32-bit word first, each word LE.
                         * Decode arithmetically to avoid alignment/endian casts. */
                        unsigned long high = u32(data + p), low = u32(data + p + FVM_AVM1_DWORD_BYTES);
                        fvm_value_number(&v, fvm_avm1_decode_double(high, low));
                        p += FVM_AVM1_DOUBLE_BYTES;
                    }
                    else if (type == FVM_AVM1_PUSH_CONSTANT8 || type == FVM_AVM1_PUSH_CONSTANT16)
                    {
                        size_t width = type == FVM_AVM1_PUSH_CONSTANT8
                                           ? FVM_AVM1_CONSTANT8_INDEX_BYTES
                                           : FVM_AVM1_CONSTANT16_INDEX_BYTES;
                        if (end - p < width)
                            return FVM_INVALID_ARGUMENT;
                        unsigned int index =
                            type == FVM_AVM1_PUSH_CONSTANT8 ? data[p] : u16(data + p);
                        p += width;
                        if (index >= constants.size())
                        {
                            if (!c->swf_loaded)
                                return FVM_INVALID_ARGUMENT;
                            /* Missing pool entries retain the initialized undefined value. */
                        }
                        else
                        {
                            FVM_Result result = own_string(c, constants[index].c_str(), &v);
                            if (result != FVM_OK)
                                return result;
                        }
                    }
                    else if (type == FVM_AVM1_PUSH_NULL)
                        v.type = FVM_VALUE_NULL;
                    else if (type == FVM_AVM1_PUSH_UNDEFINED)
                    { /* undefined */
                    }
                    else if (type == FVM_AVM1_PUSH_BOOLEAN)
                    {
                        if (end - p < 1)
                            return FVM_INVALID_ARGUMENT;
                        fvm_value_boolean(&v, data[p++]);
                    }
                    else if (type == FVM_AVM1_PUSH_INTEGER)
                    {
                        if (end - p < 4)
                            return FVM_INVALID_ARGUMENT;
                        fvm_value_number(
                            &v, (double)((u32(data + p) & FVM_UINT32_SIGN_BIT)
                                             ? (long)(u32(data + p) & FVM_UINT32_MAGNITUDE_MASK) -
                                                   FVM_INT32_MAX_LONG - 1L
                                             : (long)u32(data + p)));
                        p += 4;
                    }
                    else
                        return FVM_UNSUPPORTED;
                    stack.push_back(v);
                }
            }
            else if (a.op == FVM_AVM1_ACTION_INIT_ARRAY || a.op == FVM_AVM1_ACTION_INIT_OBJECT)
            {
                if (stack.empty())
                {
                    if (!c->swf_loaded)
                        return FVM_ERROR;
                    /* AVM1 pop on an empty operand stack produces undefined, including the count.
                     */
                    FVM_Value missing;
                    fvm_value_undefined(&missing);
                    stack.push_back(missing);
                }
                FVM_Value normalized_count;
                FVM_Result count_status =
                    fvm_exec_literal_count(c, &stack.back(), &normalized_count);
                if (count_status != FVM_OK)
                    return count_status;
                if (normalized_count.type == FVM_VALUE_UNDEFINED)
                {
                    /* A rejected signed count consumes no entry operands. */
                    stack.back() = normalized_count;
                    ++pc;
                    continue;
                }
                double count = normalized_count.number;
                unsigned int width = a.op == FVM_AVM1_ACTION_INIT_ARRAY
                                         ? FVM_AVM1_ARRAY_LITERAL_ENTRY_WIDTH
                                         : FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH;
                size_t available = stack.size() - FVM_AVM1_LITERAL_COUNT_OPERANDS;
                if (!c->swf_loaded && count * width > available)
                    return FVM_INVALID_ARGUMENT;
                size_t consumed = (size_t)count * width;
                if (consumed > available)
                    consumed = available;
                FVM_Value value;
                FVM_Result r = FVM_OK;
                if (a.op == FVM_AVM1_ACTION_INIT_ARRAY)
                {
                    fvm_detail::vector<FVM_Value> values((size_t)count);
                    for (size_t j = 0; j < values.size(); ++j)
                    {
                        if (j < available)
                            values[j] =
                                stack[stack.size() - FVM_AVM1_ARRAY_LITERAL_FIRST_VALUE - j];
                        else
                            fvm_value_undefined(&values[j]);
                    }
                    r = fvm_exec_literal_array(c, (unsigned int)count,
                                               values.empty() ? 0 : values.begin(), &value);
                }
                else
                {
                    fvm_detail::vector<FVM_ExecutionLiteralEntry> entries((size_t)count);
                    for (size_t j = 0; j < entries.size(); ++j)
                    {
                        /* Missing pair members independently pop undefined. */
                        if (j * FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH < available)
                            entries[j].value =
                                stack[stack.size() - FVM_AVM1_OBJECT_LITERAL_FIRST_VALUE -
                                      j * FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH];
                        else
                            fvm_value_undefined(&entries[j].value);
                        if (j * FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH +
                                FVM_AVM1_OBJECT_LITERAL_NAME_IN_PAIR <
                            available)
                            entries[j].name =
                                stack[stack.size() - FVM_AVM1_OBJECT_LITERAL_FIRST_NAME -
                                      j * FVM_AVM1_OBJECT_LITERAL_ENTRY_WIDTH];
                        else
                            fvm_value_undefined(&entries[j].name);
                    }
                    r = fvm_exec_literal_object(c, (unsigned int)count,
                                                entries.empty() ? 0 : entries.begin(), &value);
                }
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - FVM_AVM1_LITERAL_COUNT_OPERANDS - consumed);
                stack.push_back(value);
            }
            else if (a.op == FVM_AVM1_ACTION_EXTENDS)
            {
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_EXTENDS_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Result status = fvm_exec_extends(c,
                    &stack[stack.size() - FVM_AVM1_EXTENDS_OPERANDS], &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.resize(stack.size() - FVM_AVM1_EXTENDS_OPERANDS);
            }
            else if (a.op == FVM_AVM1_ACTION_IMPLEMENTS)
            {
                FVM_Result prepared = fvm_avm1_require_operands(
                    c->swf_loaded, stack, FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                unsigned int n = 0;
                const FVM_Value &count = stack[stack.size() - FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS];
                FVM_Result r = fvm_exec_interface_count(c, &count,
                    stack.size() - FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS, &n);
                if (r != FVM_OK)
                    return r;
                fvm_detail::vector<FVM_Value> interfaces(n);
                for (unsigned int i = 0; i < n; ++i)
                    interfaces[i] = stack[stack.size() - FVM_AVM1_IMPLEMENTS_FIRST_INTERFACE - i];
                r = fvm_exec_implements(c, &stack.back(), n, n ? interfaces.begin() : 0);
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - n - FVM_AVM1_IMPLEMENTS_FIXED_OPERANDS);
            }
            else if (a.op == FVM_AVM1_ACTION_CAST)
            {
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_CAST_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_cast(c, &stack.back(),
                    &stack[stack.size() - FVM_AVM1_CAST_OPERANDS], &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_INSTANCE_OF)
            {
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_INSTANCE_OF_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value membership;
                FVM_Result status = fvm_exec_instance_of(c,
                    &stack[stack.size() - FVM_AVM1_INSTANCE_OF_OPERANDS], &stack.back(), &membership);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = membership;
            }
            else if (a.op == FVM_AVM1_ACTION_NEXT_FRAME || a.op == FVM_AVM1_ACTION_PREVIOUS_FRAME ||
                     a.op == FVM_AVM1_ACTION_PLAY || a.op == FVM_AVM1_ACTION_STOP ||
                     a.op == FVM_AVM1_ACTION_GOTO_FRAME || a.op == FVM_AVM1_ACTION_GOTO_LABEL ||
                     a.op == FVM_AVM1_ACTION_GOTO_FRAME2)
            { /* Timeline controls use the selected display target, independently from script
                 methods. */
                FVM_Object *target = c->avm1_target_clip;
                bool playing = false;
                fvm_detail::vector<FVM_Value> frame(FVM_AVM1_TIMELINE_REQUEST_ROOT_COUNT);
                /* [GC:] Keep a coerced frame label live while resolving the target root. */
                RootGuard frame_roots(c, &frame, 0, 0, &registers);
                FVM_Result status = FVM_OK;
                if (a.op == FVM_AVM1_ACTION_GOTO_FRAME)
                {
                    if (a.length != FVM_AVM1_DIRECT_FRAME_BYTES)
                        return FVM_INVALID_ARGUMENT;
                    fvm_value_number(&frame[FVM_AVM1_TIMELINE_REQUEST_ROOT],
                                     u16(data + a.payload) + FVM_AVM1_DIRECT_FRAME_BASE);
                }
                else if (a.op == FVM_AVM1_ACTION_GOTO_LABEL)
                {
                    if (!a.length || data[a.payload + a.length - 1] ||
                        std::memchr(data + a.payload, 0, a.length - 1))
                        return FVM_INVALID_ARGUMENT;
                    status = own_string(c, (const char *)data + a.payload,
                                        &frame[FVM_AVM1_TIMELINE_REQUEST_ROOT], a.length - 1);
                }
                else if (a.op == FVM_AVM1_ACTION_GOTO_FRAME2)
                {
                    if ((a.length != FVM_AVM1_GOTO_FLAGS_BYTES &&
                         a.length != FVM_AVM1_GOTO_SCENE_BYTES) ||
                        (data[a.payload] & ~FVM_AVM1_GOTO_VALID_FLAGS) ||
                        a.length != ((data[a.payload] & FVM_AVM1_GOTO_SCENE_BIAS)
                                         ? FVM_AVM1_GOTO_SCENE_BYTES
                                         : FVM_AVM1_GOTO_FLAGS_BYTES))
                        return FVM_INVALID_ARGUMENT;
                    FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                    if (prepared != FVM_OK)
                        return prepared;
                    playing = (data[a.payload] & FVM_AVM1_GOTO_PLAY) != 0;
                    bool with_bias = (data[a.payload] & FVM_AVM1_GOTO_SCENE_BIAS) != 0;
                    unsigned long bias =
                        with_bias ? u16(data + a.payload + FVM_AVM1_GOTO_SCENE_OFFSET) : 0;
                    status = fvm_exec_frame_request(c, &stack.back(), with_bias, bias,
                                                    &frame[FVM_AVM1_TIMELINE_REQUEST_ROOT]);
                    if (status != FVM_OK)
                        return status;
                    status = variable_receiver(c, self, &target);
                    stack.pop_back();
                }
                else if (a.length)
                    return FVM_INVALID_ARGUMENT;
                if (status != FVM_OK)
                    return status;
                if (!target || target->resource_kind != FVM_RESOURCE_MOVIECLIP)
                {
                    if (!c->avm1_base_clip)
                        return FVM_UNSUPPORTED;
                }
                else
                {
                    FVM_ExecutionFrame command =
                        a.op == FVM_AVM1_ACTION_NEXT_FRAME       ? FVM_EXEC_NEXT_FRAME
                        : a.op == FVM_AVM1_ACTION_PREVIOUS_FRAME ? FVM_EXEC_PREVIOUS_FRAME
                        : a.op == FVM_AVM1_ACTION_PLAY           ? FVM_EXEC_PLAY
                        : a.op == FVM_AVM1_ACTION_STOP           ? FVM_EXEC_STOP
                                                                 : FVM_EXEC_GOTO_FRAME;
                    status = fvm_exec_frame_control(c, target, command,
                                                    a.op >= FVM_AVM1_LONG_ACTION_BASE
                                                        ? &frame[FVM_AVM1_TIMELINE_REQUEST_ROOT]
                                                        : 0,
                                                    playing);
                    if (status != FVM_OK && status != FVM_NOT_FOUND)
                        return status;
                }
            }
            else if (a.op == FVM_AVM1_ACTION_GET_PROPERTY || a.op == FVM_AVM1_ACTION_SET_PROPERTY)
            { /* GetProperty/SetProperty resolve display targets after numeric index coercion. */
                const size_t count = a.op == FVM_AVM1_ACTION_GET_PROPERTY
                                         ? FVM_AVM1_DISPLAY_READ_OPERANDS
                                         : FVM_AVM1_DISPLAY_WRITE_OPERANDS;
                FVM_Result prepared = fvm_avm1_require_operands(c->swf_loaded, stack, count);
                if (prepared != FVM_OK)
                    return prepared;
                size_t first = stack.size() - count;
                FVM_Value result;
                FVM_Result status = fvm_exec_display_property(
                    c, a.op == FVM_AVM1_ACTION_SET_PROPERTY, &stack[first],
                    &stack[first + FVM_AVM1_DISPLAY_INDEX_OFFSET],
                    a.op == FVM_AVM1_ACTION_SET_PROPERTY
                        ? &stack[first + FVM_AVM1_DISPLAY_VALUE_OFFSET]
                        : 0,
                    &result);
                if (status != FVM_OK)
                    return status;
                stack.resize(first);
                if (a.op == FVM_AVM1_ACTION_GET_PROPERTY)
                    stack.push_back(result);
            }
            else if (a.op == FVM_AVM1_ACTION_START_DRAG)
            { /* Decode only operands here; conversion, resolution and motion use shared C APIs. */
                FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, stack,
                                                              FVM_AVM1_DRAG_FIXED_OPERANDS);
                FVM_Object *start = 0;
                if (status == FVM_OK)
                    status = variable_receiver(c, self, &start);
                if (status != FVM_OK)
                    return status;
                fvm_detail::vector<FVM_Value> target(FVM_AVM1_DRAG_ROOT_COUNT);
                fvm_value_undefined(&target[FVM_AVM1_DRAG_TARGET_ROOT]);
                /* [GC:] Target resolution precedes later collecting flag/bounds conversions. */
                RootGuard target_roots(c, &target, 0, 0, &registers);
                status = fvm_exec_resolve_clip(c, start, &stack.back(), 1,
                    &target[FVM_AVM1_DRAG_TARGET_ROOT]);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                FVM_Value numeric;
                status = fvm_exec_to_number(c, 1, &stack.back(), &numeric);
                if (status != FVM_OK)
                    return status;
                bool center = numeric.number == FVM_AVM1_DRAG_ENABLED_INTEGER;
                stack.pop_back();
                status = fvm_exec_to_number(c, 1, &stack.back(), &numeric);
                if (status != FVM_OK)
                    return status;
                bool constrained = numeric.number == FVM_AVM1_DRAG_ENABLED_INTEGER;
                stack.pop_back();
                FVM_ExecutionDragBounds bounds;
                if (constrained)
                {
                    status = fvm_avm1_require_operands(c->swf_loaded, stack,
                                                        FVM_AVM1_DRAG_BOUND_OPERANDS);
                    if (status != FVM_OK)
                        return status;
                    double *limits[] = {&bounds.y_max, &bounds.x_max, &bounds.y_min, &bounds.x_min};
                    for (unsigned int i = 0; i < FVM_AVM1_DRAG_BOUND_OPERANDS; ++i)
                    {
                        status = fvm_exec_to_number(c, 0, &stack.back(), &numeric);
                        if (status != FVM_OK)
                            return status;
                        *limits[i] = numeric.number;
                        stack.pop_back();
                    }
                }
                const FVM_Value &clip = target[FVM_AVM1_DRAG_TARGET_ROOT];
                if (clip.type == FVM_VALUE_OBJECT)
                {
                    status = fvm_exec_drag_begin(c, clip.object, center, constrained ? &bounds : 0);
                    if (status != FVM_OK)
                        return status;
                }
            }
            else if (a.op == FVM_AVM1_ACTION_END_DRAG)
            {
                FVM_Result status = fvm_exec_drag_end(c);
                if (status != FVM_OK)
                    return status;
            }
            else if (a.op == FVM_AVM1_ACTION_CLONE_SPRITE)
            { /* The shared C request owns coercion, path resolution and native construction. */
                FVM_Result status = fvm_avm1_require_operands(c->swf_loaded, stack,
                                                              FVM_AVM1_CLONE_OPERANDS);
                FVM_Object *start = 0;
                if (status == FVM_OK)
                    status = variable_receiver(c, self, &start);
                if (status != FVM_OK)
                    return status;
                size_t first = stack.size() - FVM_AVM1_CLONE_OPERANDS;
                FVM_Value result;
                status = fvm_exec_duplicate_target(c, start,
                    &stack[first + FVM_AVM1_CLONE_SOURCE_OFFSET],
                    &stack[first + FVM_AVM1_CLONE_NAME_OFFSET],
                    &stack[first + FVM_AVM1_CLONE_DEPTH_OFFSET], &result);
                if (status != FVM_OK)
                    return status;
                stack.resize(first);
            }
            else if (a.op == FVM_AVM1_ACTION_REMOVE_SPRITE)
            { /* Value resolution/removal is shared; activation target changes stay in the adapter. */
                FVM_Result status = fvm_avm1_require_value(c->swf_loaded, stack);
                FVM_Object *start = 0;
                if (status == FVM_OK)
                    status = variable_receiver(c, self, &start);
                if (status == FVM_OK)
                    status = fvm_exec_remove_target(c, start, &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                if (c->avm1_target_clip)
                {
                    bool removed = false;
                    status = fvm_movieclip_scene_removed(c, c->avm1_target_clip, &removed);
                    if (status == FVM_OK && removed)
                    {
                        bool base_removed = true;
                        if (c->avm1_base_clip)
                            status = fvm_movieclip_scene_removed(c, c->avm1_base_clip, &base_removed);
                        if (status == FVM_OK)
                            c->avm1_target_clip = base_removed ? 0 : c->avm1_base_clip;
                    }
                    if (status != FVM_OK)
                        return status;
                }
            }
            else if (a.op == FVM_AVM1_ACTION_SET_TARGET || a.op == FVM_AVM1_ACTION_SET_TARGET2)
            { /* SetTarget/SetTarget2 replace the display scope without changing the this receiver.
               */
                FVM_Value path;
                if (a.op == FVM_AVM1_ACTION_SET_TARGET)
                {
                    if (!a.length || data[a.payload + a.length - 1] ||
                        std::memchr(data + a.payload, 0, a.length - 1))
                        return FVM_INVALID_ARGUMENT;
                    fvm_value_string_bytes(&path, (const char *)data + a.payload, a.length - 1);
                }
                else
                {
                    FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                    if (prepared != FVM_OK)
                        return prepared;
                    path = stack.back();
                }
                FVM_Result status = fvm_exec_select_target(c, &path);
                if (status != FVM_OK)
                    return status;
                if (a.op == FVM_AVM1_ACTION_SET_TARGET2)
                    stack.pop_back();
            }
            else if (a.op == FVM_AVM1_ACTION_GET_URL || a.op == FVM_AVM1_ACTION_GET_URL2)
            { /* GetURL/GetURL2 delegate counted requests to an explicitly registered host sink. */
                fvm_detail::vector<FVM_Value> request(FVM_AVM1_URL_REQUEST_ROOT_COUNT);
                /* [GC:] Retain original request values during the shared service's coercion. */
                RootGuard request_roots(c, &request, 0, 0, &registers);
                unsigned int flags = 0;
                FVM_Result status = FVM_OK;
                if (a.op == FVM_AVM1_ACTION_GET_URL2)
                {
                    if (a.length != FVM_AVM1_URL_FLAGS_BYTES)
                        return FVM_INVALID_ARGUMENT;
                    flags = data[a.payload];
                    if ((flags & FVM_AVM1_URL_RESERVED_MASK) ||
                        (flags & FVM_AVM1_URL_METHOD_MASK) == FVM_AVM1_URL_INVALID_METHOD)
                        return FVM_INVALID_ARGUMENT;
                    FVM_Result prepared =
                        fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_URL_OPERANDS);
                    if (prepared != FVM_OK)
                        return prepared;
                    request[FVM_AVM1_URL_REQUEST_TARGET_ROOT] = stack.back();
                    request[FVM_AVM1_URL_REQUEST_URL_ROOT] =
                        stack[stack.size() - FVM_AVM1_URL_OPERANDS];
                }
                else
                {
                    const unsigned char *payload = data + a.payload;
                    const unsigned char *separator =
                        (const unsigned char *)std::memchr(payload, 0, a.length);
                    if (!separator)
                        return FVM_INVALID_ARGUMENT;
                    size_t first = (size_t)(separator - payload);
                    size_t remaining = a.length - first - 1;
                    if (!remaining || payload[a.length - 1] ||
                        std::memchr(separator + 1, 0, remaining - 1))
                        return FVM_INVALID_ARGUMENT;
                    if (c->swf_loaded && c->swf_info.version < FVM_SWF_VERSION_6)
                        for (size_t i = 0; i < a.length; ++i)
                            if (payload[i] > FVM_AVM1_URL_LEGACY_MAX_BYTE)
                                return FVM_UNSUPPORTED;
                    status = own_string(c, (const char *)payload,
                                        &request[FVM_AVM1_URL_REQUEST_URL_ROOT], first);
                    if (status == FVM_OK)
                        status =
                            own_string(c, (const char *)separator + 1,
                                       &request[FVM_AVM1_URL_REQUEST_TARGET_ROOT], remaining - 1);
                }
                if (status != FVM_OK)
                    return status;
                status = fvm_exec_url(c, &request[FVM_AVM1_URL_REQUEST_URL_ROOT],
                                      &request[FVM_AVM1_URL_REQUEST_TARGET_ROOT], flags);
                if (status != FVM_OK)
                    return status;
                if (a.op == FVM_AVM1_ACTION_GET_URL2)
                    stack.resize(stack.size() - FVM_AVM1_URL_OPERANDS);
            }
            else if (a.op == FVM_AVM1_ACTION_CALL)
            { /* Call executes frame actions without seeking or placing frame controls. */
                if (a.length)
                    return FVM_INVALID_ARGUMENT;
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Object *scope_receiver = 0;
                FVM_Result status = variable_receiver(c, self, &scope_receiver);
                if (status == FVM_OK)
                    status = fvm_exec_call_frame(c, scope_receiver, &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
            }
            else if (a.op == FVM_AVM1_ACTION_NEW_OBJECT)
            { /* NewObject: literal loaded name lookup and shared rooted construction. */
                FVM_Result prepared = fvm_avm1_require_operands(c->swf_loaded, stack,
                                                                FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                if ((!c->swf_loaded && stack.back().type != FVM_VALUE_STRING))
                    return FVM_ERROR;
                if (c->swf_loaded)
                {
                    FVM_Value name;
                    FVM_Result name_status = fvm_exec_to_string(c, &stack.back(), &name);
                    if (name_status != FVM_OK)
                        return name_status;
                    stack.back() = name;
                }
                unsigned int argc = 0;
                FVM_Result count_status = fvm_exec_constructor_count(
                    c, &stack[stack.size() - FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS],
                    stack.size() - FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS, &argc);
                if (count_status != FVM_OK)
                    return count_status;
                fvm_detail::vector<FVM_Value> args(argc);
                for (unsigned int i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - FVM_AVM1_NEW_OBJECT_FIRST_ARGUMENT - i];
                FVM_Value value;
                FVM_Object *target = 0;
                FVM_Result result = variable_receiver(c, self, &target);
                if (result == FVM_OK)
                    result = fvm_exec_new_object(
                        c, c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back(), target,
                        c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                            ? c->script_this
                            : self,
                        locals != 0, &stack.back(), argc, argc ? args.begin() : 0, &value);
                if (result != FVM_OK)
                    return result;
                stack.resize(stack.size() - argc - FVM_AVM1_NEW_OBJECT_FIXED_OPERANDS);
                stack.push_back(value);
            }
            else if (a.op == FVM_AVM1_ACTION_TARGET_PATH)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_target_path(c, &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_ENUMERATE || a.op == FVM_AVM1_ACTION_ENUMERATE2)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value target = stack.back();
                FVM_Result status = FVM_OK;
                if (a.op == FVM_AVM1_ACTION_ENUMERATE)
                {
                    if (c->swf_loaded)
                    {
                        status = fvm_exec_to_string(c, &stack.back(), &target);
                        if (status != FVM_OK)
                            return status;
                        /* [GC:] Keep the converted name alive through variable lookup getters. */
                        stack.back() = target;
                    }
                    else if (target.type != FVM_VALUE_STRING)
                        return FVM_INVALID_ARGUMENT;
                    status = lookup_variable_value(c, locals, self, target, &target);
                }
                if (status != FVM_OK)
                    return status;
                FVM_Value snapshot;
                status = fvm_exec_enumerate(c, &target, &snapshot);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                FVM_Value end;
                fvm_value_undefined(&end);
                if (!c->swf_loaded)
                    end.type = FVM_VALUE_NULL;
                stack.push_back(end);
                for (size_t i = snapshot.object->indexed.size(); i > 0; --i)
                    stack.push_back(snapshot.object->indexed.find((unsigned long)i - 1)->second);
            }
            else if (a.op == FVM_AVM1_ACTION_STRING_EQUALS || a.op == FVM_AVM1_ACTION_STRING_ADD ||
                     a.op == FVM_AVM1_ACTION_STRING_LESS || a.op == FVM_AVM1_ACTION_STRING_GREATER)
            { /* Explicit string operations use shared rooted value services. */
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_STRING_BINARY_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                const FVM_Value *left = &stack[stack.size() - FVM_AVM1_STRING_BINARY_OPERANDS];
                FVM_Value output;
                FVM_Result status;
                if (a.op == FVM_AVM1_ACTION_STRING_ADD)
                    status = fvm_exec_string_add(c, left, &stack.back(), &output);
                else
                    status = fvm_exec_string_compare(
                        c,
                        a.op == FVM_AVM1_ACTION_STRING_EQUALS ? FVM_EXEC_EQUAL
                        : a.op == FVM_AVM1_ACTION_STRING_LESS ? FVM_EXEC_LESS
                                                              : FVM_EXEC_GREATER,
                        left, &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_STRING_LENGTH ||
                     a.op == FVM_AVM1_ACTION_MB_STRING_LENGTH)
            { /* StringLength/MBStringLength share value-based length semantics. */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value output;
                FVM_Result status = fvm_exec_string_length(c, &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_STRING_EXTRACT ||
                     a.op == FVM_AVM1_ACTION_MB_STRING_EXTRACT)
            { /* StringExtract/MBStringExtract convert length, one-based start, then input text. */
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_STRING_EXTRACT_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value output;
                FVM_Result status = fvm_exec_string_extract(
                    c, &stack[stack.size() - FVM_AVM1_STRING_EXTRACT_OPERANDS],
                    &stack[stack.size() - FVM_AVM1_STRING_EXTRACT_START_OFFSET], &stack.back(),
                    &output);
                if (status != FVM_OK)
                    return status;
                stack.resize(stack.size() - (FVM_AVM1_STRING_EXTRACT_OPERANDS - 1));
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_CHAR_TO_ASCII ||
                     a.op == FVM_AVM1_ACTION_MB_CHAR_TO_ASCII)
            { /* CharToAscii/MBCharToAscii share value-based character reads. */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value output;
                FVM_Result status = fvm_exec_char_code(c, &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_ASCII_TO_CHAR ||
                     a.op == FVM_AVM1_ACTION_MB_ASCII_TO_CHAR)
            { /* AsciiToChar/MBAsciiToChar share value-based character construction. */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value output;
                FVM_Result status = fvm_exec_char_from_code(c, &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_TO_INTEGER)
            { /* ToInteger uses signed Int32 wrapping, not an unsafe floating cast. */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_to_number(c, 1, &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_TYPE_OF || a.op == FVM_AVM1_ACTION_TO_NUMBER ||
                     a.op == FVM_AVM1_ACTION_TO_STRING)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value input = stack.back(), output;
                FVM_Result r = FVM_OK;
                if (a.op == FVM_AVM1_ACTION_TYPE_OF)
                    r = fvm_exec_type_of(c, &input, &output);
                else if (a.op == FVM_AVM1_ACTION_TO_STRING)
                    r = fvm_exec_to_string(c, &input, &output);
                else
                    r = fvm_exec_to_number(c, 0, &input, &output);
                if (r != FVM_OK)
                    return r;
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_ADD2 || a.op == FVM_AVM1_ACTION_LESS2 ||
                     a.op == FVM_AVM1_ACTION_EQUALS2 || a.op == FVM_AVM1_ACTION_STRICT_EQUALS ||
                     a.op == FVM_AVM1_ACTION_GREATER)
            {
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_COMPARE_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_ExecutionCompare operation = a.op == FVM_AVM1_ACTION_ADD2 ? FVM_EXEC_ADD_DYNAMIC
                                                 : a.op == FVM_AVM1_ACTION_LESS2   ? FVM_EXEC_LESS
                                                 : a.op == FVM_AVM1_ACTION_EQUALS2 ? FVM_EXEC_EQUAL
                                                 : a.op == FVM_AVM1_ACTION_STRICT_EQUALS
                                                     ? FVM_EXEC_STRICT_EQUAL
                                                     : FVM_EXEC_GREATER;
                FVM_Value output;
                FVM_Result status =
                    fvm_exec_compare(c, operation, &stack[stack.size() - FVM_AVM1_COMPARE_OPERANDS],
                                     &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = output;
            }
            else if (a.op == FVM_AVM1_ACTION_WAIT_FOR_FRAME ||
                     a.op == FVM_AVM1_ACTION_WAIT_FOR_FRAME2)
            {
                bool direct = a.op == FVM_AVM1_ACTION_WAIT_FOR_FRAME;
                if (a.length != (direct ? FVM_AVM1_WAIT_DIRECT_BYTES : FVM_AVM1_WAIT_VALUE_BYTES))
                    return FVM_INVALID_ARGUMENT;
                FVM_Value frame, available;
                if (direct)
                    fvm_value_number(&frame, u16(data + a.payload));
                else
                {
                    if (stack.empty())
                    {
                        if (!c->swf_loaded)
                            return FVM_ERROR;
                        fvm_value_undefined(&frame);
                    }
                    else
                        frame = stack.back();
                }
                FVM_Result status = fvm_exec_frame_loaded(c,
                    direct ? FVM_EXEC_FRAME_CHECK_DIRECT : FVM_EXEC_FRAME_CHECK_VALUE, &frame, &available);
                if (status != FVM_OK)
                    return status;
                if (!direct && !stack.empty())
                    stack.pop_back();
                if (!available.boolean)
                {
                    size_t skip = data[a.payload + (direct ? FVM_AVM1_WAIT_DIRECT_SKIP_OFFSET : 0)];
                    size_t remaining = actions.size() - pc - 1;
                    if (skip >= remaining)
                        return FVM_OK;
                    pc += skip;
                }
            }
            else if (a.op == FVM_AVM1_ACTION_GET_TIME)
            {
                FVM_Value result;
                FVM_Result status = fvm_exec_get_time(c, &result);
                if (status != FVM_OK)
                    return status;
                stack.push_back(result);
            }
            else if (a.op == FVM_AVM1_ACTION_RANDOM_NUMBER)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_random_number(c, &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_INCREMENT || a.op == FVM_AVM1_ACTION_DECREMENT)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_step(c, a.op == FVM_AVM1_ACTION_DECREMENT,
                                                  &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_MODULO ||
                     (a.op >= FVM_AVM1_ACTION_BIT_AND && a.op <= FVM_AVM1_ACTION_BIT_URSHIFT))
            {
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_NUMERIC_BINARY_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_ExecutionNumeric operation =
                    a.op == FVM_AVM1_ACTION_MODULO       ? FVM_EXEC_REMAINDER
                    : a.op == FVM_AVM1_ACTION_BIT_AND    ? FVM_EXEC_BIT_AND
                    : a.op == FVM_AVM1_ACTION_BIT_OR     ? FVM_EXEC_BIT_OR
                    : a.op == FVM_AVM1_ACTION_BIT_XOR    ? FVM_EXEC_BIT_XOR
                    : a.op == FVM_AVM1_ACTION_BIT_LSHIFT ? FVM_EXEC_SHIFT_LEFT
                    : a.op == FVM_AVM1_ACTION_BIT_RSHIFT ? FVM_EXEC_SHIFT_RIGHT
                                                         : FVM_EXEC_SHIFT_UNSIGNED;
                FVM_Value result;
                FVM_Result status = fvm_exec_numeric(
                    c, operation, &stack[stack.size() - FVM_AVM1_NUMERIC_BINARY_OPERANDS],
                    &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_GET_MEMBER || a.op == FVM_AVM1_ACTION_SET_MEMBER)
            {
                size_t count = a.op == FVM_AVM1_ACTION_GET_MEMBER ? FVM_AVM1_GET_MEMBER_OPERANDS
                                                                  : FVM_AVM1_SET_MEMBER_OPERANDS;
                FVM_Result prepared = fvm_avm1_require_operands(c->swf_loaded, stack, count);
                if (prepared != FVM_OK)
                    return prepared;
                size_t first = stack.size() - count;
                FVM_Value result;
                FVM_Result status =
                    fvm_exec_member(c, a.op == FVM_AVM1_ACTION_SET_MEMBER, &stack[first],
                                    &stack[first + FVM_AVM1_MEMBER_NAME_OFFSET],
                                    a.op == FVM_AVM1_ACTION_SET_MEMBER
                                        ? &stack[first + FVM_AVM1_MEMBER_VALUE_OFFSET]
                                        : 0,
                                    &result);
                if (status != FVM_OK)
                    return status;
                stack.resize(first);
                if (a.op == FVM_AVM1_ACTION_GET_MEMBER)
                    stack.push_back(result);
            }
            else if (a.op == FVM_AVM1_ACTION_DELETE)
            { /* Delete: owner and property name; absent/inherited members report false. */
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_DELETE_MEMBER_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_delete_member(
                    c, &stack[stack.size() - FVM_AVM1_DELETE_MEMBER_OPERANDS], &stack.back(),
                    &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_DELETE2)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Object *target = 0;
                FVM_Result status = variable_receiver(c, self, &target);
                FVM_Value result;
                if (status == FVM_OK)
                    status = fvm_exec_delete_reference(c, c->lexical_scopes.empty()
                        ? 0 : c->lexical_scopes.back(), target,
                        c->script_this ? c->script_this : self, &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_PUSH_DUPLICATE)
            { /* PushDuplicate */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                stack.push_back(stack.back());
            }
            else if (a.op == FVM_AVM1_ACTION_STACK_SWAP)
            { /* StackSwap */
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_SWAP_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value tmp = stack.back();
                stack.back() = stack[stack.size() - FVM_AVM1_SWAP_OPERANDS];
                stack[stack.size() - FVM_AVM1_SWAP_OPERANDS] = tmp;
            }
            else if (a.op == FVM_AVM1_ACTION_NOT)
            { /* Not */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status = fvm_exec_logical(c, FVM_EXEC_LOGICAL_NOT, &stack.back(), 0, &result);
                if (status != FVM_OK)
                    return status;
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_EQUALS || a.op == FVM_AVM1_ACTION_LESS ||
                     a.op == FVM_AVM1_ACTION_AND || a.op == FVM_AVM1_ACTION_OR)
            {
                /* [GC:] Keep both operands in the rooted stack through native coercion. */
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_LEGACY_BINARY_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Value result;
                FVM_Result status;
                if (a.op == FVM_AVM1_ACTION_EQUALS || a.op == FVM_AVM1_ACTION_LESS)
                    status = fvm_exec_legacy_compare(
                        c,
                        a.op == FVM_AVM1_ACTION_EQUALS ? FVM_EXEC_LEGACY_EQUAL
                                                       : FVM_EXEC_LEGACY_LESS,
                        &stack[stack.size() - FVM_AVM1_LEGACY_BINARY_OPERANDS], &stack.back(),
                        &result);
                else
                    status = fvm_exec_logical(
                        c, a.op == FVM_AVM1_ACTION_AND ? FVM_EXEC_LOGICAL_AND : FVM_EXEC_LOGICAL_OR,
                        &stack[stack.size() - FVM_AVM1_LEGACY_BINARY_OPERANDS], &stack.back(),
                        &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_TRACE)
            { /* Trace always coerces its operand, even when host delivery is disabled. */
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Result status = fvm_exec_trace(c, &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
            }
            else if (a.op == FVM_AVM1_ACTION_POP)
            { /* Pop */
                if (stack.empty())
                {
                    if (!c->swf_loaded)
                        return FVM_ERROR;
                }
                else
                    stack.pop_back();
            }
            else if (a.op == FVM_AVM1_ACTION_DEFINE_LOCAL || a.op == FVM_AVM1_ACTION_DEFINE_LOCAL2)
            {
                size_t operands = a.op == FVM_AVM1_ACTION_DEFINE_LOCAL
                    ? FVM_AVM1_DECLARE_INITIALIZED_OPERANDS : FVM_AVM1_DECLARE_ONLY_OPERANDS;
                FVM_Result prepared = fvm_avm1_require_operands(c->swf_loaded, stack, operands);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_Object *target = 0;
                FVM_Result status = variable_receiver(c, self, &target);
                if (status == FVM_OK)
                    status = fvm_exec_variable_declare(c, c->lexical_scopes.empty()
                        ? 0 : c->lexical_scopes.back(), local_activation(c, locals),
                        target && target->resource_kind == FVM_RESOURCE_MOVIECLIP ? target : c->global,
                        c->script_this ? c->script_this : self,
                        a.op == FVM_AVM1_ACTION_DEFINE_LOCAL
                            ? FVM_EXEC_DECLARE_INITIALIZED : FVM_EXEC_DECLARE_UNINITIALIZED,
                        &stack[stack.size() - operands],
                        a.op == FVM_AVM1_ACTION_DEFINE_LOCAL ? &stack.back() : 0);
                if (status != FVM_OK)
                    return status;
                stack.resize(stack.size() - operands);
            }
            else if (a.op == FVM_AVM1_ACTION_GET_VARIABLE)
            {
                FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                if (prepared != FVM_OK)
                    return prepared;
                if (!c->swf_loaded && stack.back().type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                FVM_Object *target = 0;
                FVM_Result r = variable_receiver(c, self, &target);
                FVM_Value value;
                if (r == FVM_OK)
                    r = fvm_exec_get_variable(
                        c, c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back(), target,
                        c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                            ? c->script_this
                            : self,
                        locals != 0, &stack.back(), &value);
                if (r != FVM_OK)
                    return r;
                stack.back() = value;
            }
            else if (a.op == FVM_AVM1_ACTION_SET_VARIABLE)
            {
                FVM_Result prepared = fvm_avm1_require_operands(
                    c->swf_loaded, stack, FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT);
                if (prepared != FVM_OK)
                    return prepared;
                if (!c->swf_loaded &&
                    stack[stack.size() - FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT].type !=
                        FVM_VALUE_STRING)
                    return FVM_ERROR;
                FVM_Object *target = 0;
                FVM_Result status = variable_receiver(c, self, &target);
                if (status == FVM_OK)
                    status = fvm_exec_variable_set(c, c->lexical_scopes.empty()
                        ? 0 : c->lexical_scopes.back(),
                        target && target->resource_kind == FVM_RESOURCE_MOVIECLIP ? target : c->global,
                        c->script_this ? c->script_this : self,
                        &stack[stack.size() - FVM_AVM1_VARIABLE_WRITE_OPERAND_COUNT], &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.pop_back();
            }
            else if (a.op == FVM_AVM1_ACTION_ADD || a.op == FVM_AVM1_ACTION_SUBTRACT ||
                     a.op == FVM_AVM1_ACTION_MULTIPLY || a.op == FVM_AVM1_ACTION_DIVIDE)
            {
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_NUMERIC_BINARY_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                FVM_ExecutionNumeric operation =
                    a.op == FVM_AVM1_ACTION_ADD        ? FVM_EXEC_ADD
                    : a.op == FVM_AVM1_ACTION_SUBTRACT ? FVM_EXEC_SUBTRACT
                    : a.op == FVM_AVM1_ACTION_MULTIPLY ? FVM_EXEC_MULTIPLY
                                                       : FVM_EXEC_DIVIDE;
                FVM_Value result;
                FVM_Result status = fvm_exec_numeric(
                    c, operation, &stack[stack.size() - FVM_AVM1_NUMERIC_BINARY_OPERANDS],
                    &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == FVM_AVM1_ACTION_CALL_METHOD || a.op == FVM_AVM1_ACTION_NEW_METHOD)
            { /* CallMethod: args, argc, receiver, method name */
                FVM_Result prepared =
                    fvm_avm1_require_operands(c->swf_loaded, stack, FVM_AVM1_METHOD_FIXED_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                if ((!c->swf_loaded && stack.back().type != FVM_VALUE_STRING &&
                     stack.back().type != FVM_VALUE_NULL &&
                     stack.back().type != FVM_VALUE_UNDEFINED))
                    return FVM_ERROR;
                unsigned int argc = 0;
                FVM_Result count_status = fvm_exec_argument_count(
                    c, &stack[stack.size() - FVM_AVM1_METHOD_FIXED_OPERANDS],
                    stack.size() - FVM_AVM1_METHOD_FIXED_OPERANDS, &argc);
                if (count_status != FVM_OK)
                    return count_status;
                const FVM_Value &owner = stack[stack.size() - FVM_AVM1_METHOD_OWNER_OFFSET];
                if ((a.op == FVM_AVM1_ACTION_CALL_METHOD || c->swf_loaded) &&
                    (owner.type == FVM_VALUE_NULL || owner.type == FVM_VALUE_UNDEFINED))
                {
                    /* Preserve the allocation-free discard of a nullish call's operands. */
                    stack.resize(stack.size() - argc - FVM_AVM1_METHOD_FIXED_OPERANDS);
                    FVM_Value missing;
                    fvm_value_undefined(&missing);
                    stack.push_back(missing);
                    ++pc;
                    continue;
                }
                fvm_detail::vector<FVM_Value> args(argc);
                for (i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - FVM_AVM1_METHOD_FIRST_ARGUMENT - i];
                FVM_Value result;
                FVM_Result r = fvm_exec_call_method(
                    c, a.op == FVM_AVM1_ACTION_NEW_METHOD,
                    &stack[stack.size() - FVM_AVM1_METHOD_OWNER_OFFSET], &stack.back(), self, argc,
                    argc ? args.begin() : 0, &result);
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - argc - FVM_AVM1_METHOD_FIXED_OPERANDS);
                stack.push_back(result);
            }
            else if (a.op == FVM_AVM1_ACTION_CALL_FUNCTION)
            { /* CallFunction: args, argc, name (top) */
                FVM_Result prepared = fvm_avm1_require_operands(c->swf_loaded, stack,
                                                                FVM_AVM1_NAMED_CALL_FIXED_OPERANDS);
                if (prepared != FVM_OK)
                    return prepared;
                if ((!c->swf_loaded && stack.back().type != FVM_VALUE_STRING))
                    return FVM_ERROR;
                if (c->swf_loaded)
                {
                    FVM_Value name;
                    FVM_Result name_status = fvm_exec_to_string(c, &stack.back(), &name);
                    if (name_status != FVM_OK)
                        return name_status;
                    stack.back() = name;
                }
                unsigned int argc = 0;
                FVM_Result count_status = fvm_exec_argument_count(
                    c, &stack[stack.size() - FVM_AVM1_NAMED_CALL_FIXED_OPERANDS],
                    stack.size() - FVM_AVM1_NAMED_CALL_FIXED_OPERANDS, &argc);
                if (count_status != FVM_OK)
                    return count_status;
                fvm_detail::vector<FVM_Value> args(argc);
                for (i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - FVM_AVM1_NAMED_CALL_FIRST_ARGUMENT - i];
                FVM_Value result;
                FVM_Object *target = 0, *default_receiver = c->global;
                FVM_Result r = variable_receiver(c, self, &target);
                if (r == FVM_OK && c->swf_loaded && c->avm1_base_clip)
                    r = variable_receiver(c, c->avm1_base_clip, &default_receiver);
                if (r == FVM_OK)
                    r = fvm_exec_call_function(
                        c, c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back(), target,
                        c->swf_loaded && c->swf_info.version >= FVM_SWF_VERSION_5 && c->script_this
                            ? c->script_this
                            : self,
                        default_receiver, locals != 0, &stack.back(), argc, argc ? args.begin() : 0,
                        &result);
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - argc - FVM_AVM1_NAMED_CALL_FIXED_OPERANDS);
                stack.push_back(result);
            }
            else if (a.op == FVM_AVM1_ACTION_JUMP || a.op == FVM_AVM1_ACTION_IF)
            { /* Jump / If */
                if (a.length != FVM_AVM1_BRANCH_BYTES)
                    return FVM_INVALID_ARGUMENT;
                int take = 1;
                if (a.op == FVM_AVM1_ACTION_IF)
                {
                    FVM_Result prepared = fvm_avm1_require_value(c->swf_loaded, stack);
                    if (prepared != FVM_OK)
                        return prepared;
                    bool converted = false;
                    FVM_Result status = action_truth(c, stack.back(), converted);
                    if (status != FVM_OK)
                        return status;
                    stack.pop_back();
                    take = converted;
                }
                if (take)
                {
                    long delta = (long)u16(data + a.payload);
                    if (delta >= FVM_AVM1_BRANCH_SIGN_BIT)
                        delta -= FVM_AVM1_BRANCH_MODULUS;
                    if (delta < 0 && (size_t)(-delta) > a.next)
                        return FVM_INVALID_ARGUMENT;
                    size_t target = delta < 0 ? a.next - (size_t)(-delta) : a.next + (size_t)delta;
                    for (i = 0; i < actions.size(); ++i)
                        if (actions[i].offset == target)
                            break;
                    if (i == actions.size())
                        return FVM_INVALID_ARGUMENT;
                    pc = i;
                    continue;
                }
            }
            else
                return FVM_UNSUPPORTED;
            ++pc;
        }
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
    return shared ? FVM_OK : FVM_INVALID_ARGUMENT;
}
} // namespace
/* Execute avm1. */
extern "C" FVM_Result fvm_avm1_execute(FVM_Context *c, const void *data, size_t size)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result r = c->swf_loaded ? fvm_avm1_inspect_block(data, size, &info)
                                 : fvm_avm1_inspect(data, size, &info);
    if (r != FVM_OK)
        return r;
    bool handled = false;
    FVM_Result native = fvm_execution_try_root(c, data, size, c->global, &handled);
    if (handled)
        return native;
    FVM_Value root_this;
    fvm_value_undefined(&root_this);
    root_this.type = FVM_VALUE_OBJECT;
    root_this.object = c->global;
    return execute(c, (const unsigned char *)data, size, 0, 0, c->global, 0, 0, 0, 0, c->swf_loaded,
                   &root_this);
}
/* [GC:] Replace a synchronous host trace sink after validating its retained capture. */
extern "C" FVM_Result fvm_avm1_set_trace_rooted(FVM_Context *c, FVM_NativeFunction callback,
                                                void *userdata, FVM_Object *capture)
{
    if (!c || c->destroying || c->gc_collecting || (capture && capture->owner != c) ||
        (!callback && (userdata || capture)))
        return FVM_INVALID_ARGUMENT;
    FVM_NativeEntry entry;
    entry.callback = callback;
    entry.userdata = userdata;
    entry.root = capture;
    c->avm1_trace = entry;
    return FVM_OK;
}

/* [GC:] Replace the host request sink only after validating its retained capture. */
extern "C" FVM_Result fvm_avm1_set_url_rooted(FVM_Context *c, FVM_NativeFunction callback,
                                              void *userdata, FVM_Object *capture)
{
    if (!c || c->destroying || c->gc_collecting || (capture && capture->owner != c) ||
        (!callback && (userdata || capture)))
        return FVM_INVALID_ARGUMENT;
    FVM_NativeEntry entry;
    entry.callback = callback;
    entry.userdata = userdata;
    entry.root = capture;
    c->avm1_url = entry;
    return FVM_OK;
}

/* Execute swf actions. */
extern "C" FVM_Result fvm_swf_execute_actions(FVM_Context *c)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    if (!c->swf_loaded)
        return FVM_ERROR;

    /* Initialization / Инициализация локального состояния. */
    const unsigned char *p = &c->swf_bytes[0];
    size_t size = c->swf_bytes.size();
    size_t rect = (FVM_SWF_RECT_WIDTH_BITS + FVM_SWF_RECT_COORDINATES *
                   (p[FVM_SWF_PREFIX_BYTES] >> FVM_SWF_RECT_WIDTH_SHIFT) +
                   FVM_SWF_BYTE_BITS - 1) / FVM_SWF_BYTE_BITS;
    size_t pos = FVM_SWF_PREFIX_BYTES + rect + FVM_SWF_FRAME_FIELDS_BYTES;
    while (pos < size)
    {
        unsigned int header = u16(p + pos);
        pos += FVM_SWF_TAG_HEADER_BYTES;
        unsigned int code = header >> FVM_SWF_TAG_CODE_SHIFT;
        size_t len = header & FVM_SWF_TAG_LENGTH_MASK;
        if (len == FVM_SWF_LONG_LENGTH_SENTINEL)
        {
            len = (size_t)u32(p + pos);
            pos += FVM_SWF_LONG_LENGTH_BYTES;
        }
        if (code == FVM_SWF_TAG_DO_ACTION)
        {
            FVM_Result r = fvm_avm1_execute(c, p + pos, len);
            if (r != FVM_OK)
                return r;
        }
        pos += len;
        if (code == FVM_SWF_TAG_END)
            break;
    }
    return FVM_OK;
}

/* Call avm1. */
/* Invoke a legacy terminated AVM1 name through the counted call bridge. */
extern "C" FVM_Result fvm_avm1_call(FVM_Context *c, const char *name, FVM_Object *self,
                                    unsigned int argc, const FVM_Value *args, FVM_Value *out)
{
    return name ? fvm_avm1_call_bytes(c, name, std::strlen(name), self, argc, args, out)
                : FVM_INVALID_ARGUMENT;
}
/* [GC:] Resolve full named function identities while retaining native/script call roots. */
extern "C" FVM_Result fvm_avm1_call_bytes(FVM_Context *c, const char *name, size_t size,
                                          FVM_Object *self, unsigned int argc,
                                          const FVM_Value *args, FVM_Value *out)
{
    if (!c || !name || !out || (self && self->owner != c) || (argc && !args))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < argc; ++i)
        if (fvm_validate_value(c, args + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    try
    {
        fvm_detail::string key;
        FVM_Result result = fvm_property_key(name, size, key);
        if (result != FVM_OK)
            return result;
        FVM_Value function;
        result = fvm_get_global_bytes(c, key.data(), key.size(), &function);
        if (result != FVM_OK)
            return result;
        if (function.type == FVM_VALUE_OBJECT && function.object->invocable.callback)
            return fvm_function_invoke(c, function.object, self, argc, args, out);
        fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::iterator found =
            c->script_functions.find(key);
        return found == c->script_functions.end()
                   ? FVM_NOT_FOUND
                   : fvm_avm1_call_script(c, key, found->second, argc, args,
                                          self ? self : c->global, out);
    }
    catch (const std::bad_alloc &)
    {
        fvm_value_undefined(out);
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        fvm_value_undefined(out);
        return FVM_ERROR;
    }
}

/* Enter the private interpreter from the shared, value-based function activation path. */
FVM_Result fvm_avm1_interpret_function(FVM_Context *c, const FVM_ScriptFunction &snapshot,
                                       FVM_Value *result,
                                       fvm_detail::map<fvm_detail::string, FVM_Value> *frame,
                                       FVM_Object *receiver, Execution *execution,
                                       const FVM_Value *implicit_this)
{
    return execute(c, snapshot.body.begin(), snapshot.body.size(), result, frame, receiver,
                   &snapshot.constants, execution, 0, 0, false, implicit_this);
}

/* Execute avm1 receiver. */
extern "C" FVM_Result fvm_avm1_execute_receiver(FVM_Context *c, const void *data, size_t size,
                                                FVM_Object *receiver)
{
    if (!c || !receiver || receiver->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result r = c->swf_loaded ? fvm_avm1_inspect_block(data, size, &info)
                                 : fvm_avm1_inspect(data, size, &info);
    if (r != FVM_OK)
        return r;
    bool handled = false;
    FVM_Result native = fvm_execution_try_root(c, data, size, receiver, &handled);
    if (handled)
        return native;
    FVM_Value root_this;
    fvm_value_undefined(&root_this);
    root_this.type = FVM_VALUE_OBJECT;
    root_this.object = receiver;
    return execute(c, (const unsigned char *)data, size, 0, 0, receiver, 0, 0, 0, 0, c->swf_loaded,
                   &root_this);
}

/* Execute an exact stored frame block independently of the receiving context's load state. */
FVM_Result fvm_avm1_execute_block_receiver(FVM_Context *c, const void *data, size_t size,
                                           FVM_Object *receiver)
{
    if (!c || !receiver || receiver->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result result = fvm_avm1_inspect_block(data, size, &info);
    if (result != FVM_OK)
        return result;
    /* Stored frame identities keep their own interpreter breakpoint exclusion. */
    if (!fvm_debug_has_breakpoints(c, c->debug_function))
    {
        bool handled = false;
        FVM_Result native = fvm_execution_try_root(c, data, size, receiver, &handled, true);
        if (handled)
            return native;
    }
    FVM_Value root_this;
    fvm_value_undefined(&root_this);
    root_this.type = FVM_VALUE_OBJECT;
    root_this.object = receiver;
    return execute(c, (const unsigned char *)data, size, 0, 0, receiver, 0, 0, 0,
                   0, true, &root_this);
}
