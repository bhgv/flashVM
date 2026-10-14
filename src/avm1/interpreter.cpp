/* FlashVM: AVM1 bytecode decoding and execution: interpreter.
 * File: src/avm1/interpreter.cpp.
 */

#include "function_record.hpp"
#include "handlers/script_call.hpp"
#include "block.hpp"
#include "coercion.hpp"
#include "target_scope.hpp"
#include "../standard/function_state.hpp"
#include "../standard/inheritance.hpp"
#include "../standard/super.hpp"
#include "../standard/base_state.hpp"
#include "../standard/primitive_state.hpp"
#include "../standard/movieclip_state.hpp"
#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "../standard/string_order.hpp"
#include "../../external/flash_case/case_table.hpp"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include "try_record.hpp"
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
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

/* Compute and return u32. */
static unsigned long u32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}
/* Convert a numeric operand to the low 32 unsigned bits without undefined float-to-int casts. */
static unsigned long integer_bits(double value)
{
    if (value != value || value == std::numeric_limits<double>::infinity() ||
        value == -std::numeric_limits<double>::infinity())
        return 0;
    value = value < 0 ? std::ceil(value) : std::floor(value);
    value = std::fmod(value, 4294967296.0);
    return (unsigned long)(value < 0 ? value + 4294967296.0 : value);
}
/* Represent a low 32-bit result as a signed ActionScript Number on both 32/64-bit hosts. */
static double signed_bits(unsigned long value)
{
    value &= 0xffffffffUL;
    return value >= 0x80000000UL ? (double)value - 4294967296.0 : (double)value;
}
/* Bytecode buffers and function snapshots can disappear after execution.
 * Any string escaping into VM properties must have context lifetime. */
/* Compute and return truth. */
static bool truth(const FVM_Value &value)
{
    switch (value.type)
    {
    case FVM_VALUE_BOOLEAN:
        return value.boolean != 0;
    case FVM_VALUE_NUMBER:
        return value.number != 0.0 && value.number == value.number;
    case FVM_VALUE_STRING:
        return value.string && fvm_string_size(value) != 0;
    case FVM_VALUE_OBJECT:
        return true;
    default:
        return false;
    }
}

/* Use numeric string truth in loaded SWFs below version 7, preserving conversion failures. */
static FVM_Result action_truth(FVM_Context *context, const FVM_Value &value, bool &out)
{
    return fvm_avm1_boolean(context, value, &out);
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

/* Match the implicit this keyword using the SWF scope/version case rules.
 * Counted WTF-8 decoding and Flash UCS2 lowercase mappings preserve NULs and
 * reject supplementary characters instead of treating them as ASCII letters. */
static bool this_keyword(FVM_Context *c, bool local_scope, const fvm_detail::string &name)
{
    if (!c->swf_loaded || c->swf_info.version < 5 || c->swf_info.version >= 7 ||
        (c->swf_info.version <= 5 && local_scope))
        return name == fvm_detail::string("this");
    size_t offset = 0;
    const char expected[] = "this";
    for (unsigned int i = 0; i < 4; ++i)
    {
        unsigned long unit = 0;
        if (fvm_string_read(name.data(), name.size(), offset, unit) != FVM_OK)
            return false;
        size_t first = 0, last = sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]);
        while (first < last)
        {
            size_t middle = first + (last - first) / 2;
            if (fvm_flash_lowercase[middle][0] < unit)
                first = middle + 1;
            else
                last = middle;
        }
        if (first < sizeof(fvm_flash_lowercase) / sizeof(fvm_flash_lowercase[0]) &&
            fvm_flash_lowercase[first][0] == unit)
            unit = fvm_flash_lowercase[first][1];
        if (unit != (unsigned char)expected[i])
            return false;
    }
    return offset == name.size();
}

/* Look up variable. */
static FVM_Result lookup_variable(FVM_Context *c,
                                  fvm_detail::map<fvm_detail::string, FVM_Value> *locals,
                                  FVM_Object *self, const fvm_detail::string &name, FVM_Value *out,
                                  bool *defined = 0, FVM_Object **call_receiver = 0)
{
    if (defined)
        *defined = true;
    if (call_receiver)
        *call_receiver = c->global;
    /* Loaded SWF5+ resolves implicit this before ordinary local bindings.
     * This changes lookup precedence, not the unresolved inherited this-cell model. */
    if (self && c->swf_loaded && c->swf_info.version >= 5 && this_keyword(c, locals != 0, name))
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = c->script_this ? c->script_this : self;
        return FVM_OK;
    }
    if (locals)
    {
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator local = locals->find(name);
        if (local != locals->end())
        {
            *out = local->second;
            return FVM_OK;
        }
    }
    if (name == fvm_detail::string("_global"))
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = c->global;
        return FVM_OK;
    }
    if (self && this_keyword(c, locals != 0, name))
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = self;
        return FVM_OK;
    }

    /* Initialization / Инициализация локального состояния. */
    FVM_Object *binding = c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
    unsigned int depth = 0;
    while (binding && depth++ < 1024)
    {
        if (binding->properties.find(name) != binding->properties.end())
            return fvm_object_get_bytes(c, binding, name.data(), name.size(), out);
        binding = binding->prototype;
    }
    if (binding)
        return FVM_ERROR;

    /* Computation / Вычисление и обновление состояния. */
    FVM_Object *scope_receiver = 0;
    FVM_Result selected = variable_receiver(c, self, &scope_receiver);
    if (selected != FVM_OK)
        return selected;
    binding = scope_receiver && scope_receiver != c->global ? scope_receiver : 0;
    depth = 0;
    while (binding && depth++ < 1024)
    {
        if (binding->properties.find(name) != binding->properties.end() ||
            binding->methods.find(name) != binding->methods.end() ||
            binding->accessors.find(name) != binding->accessors.end())
        {
            if (call_receiver)
                *call_receiver = scope_receiver;
            return fvm_object_get_bytes(c, scope_receiver, name.data(), name.size(), out);
        }
        binding = binding->prototype;
    }
    if (binding)
        return FVM_ERROR;
    FVM_Result r = fvm_get_global_bytes(c, name.data(), name.size(), out);
    if (r == FVM_OK && defined && out->type == FVM_VALUE_UNDEFINED)
    {
        int own = 0;
        r = fvm_object_has_own_bytes(c, c->global, name.data(), name.size(), &own);
        *defined = own != 0;
    }
    return r;
}

/* Declare a local cell or top-level clip/global binding without erasing an existing var
 * declaration. */
static FVM_Result declare_variable(FVM_Context *context,
                                   fvm_detail::map<fvm_detail::string, FVM_Value> *locals,
                                   FVM_Object *self, const fvm_detail::string &name,
                                   const FVM_Value &value, bool initialized)
{
    if (locals)
    {
        if (initialized || locals->find(name) == locals->end())
            (*locals)[name] = value;
        return FVM_OK;
    }
    FVM_Object *scope_receiver = 0;
    FVM_Result selected = variable_receiver(context, self, &scope_receiver);
    if (selected != FVM_OK)
        return selected;
    FVM_Object *destination =
        scope_receiver && scope_receiver->resource_kind == 40 ? scope_receiver : context->global;
    if (!initialized)
    {
        FVM_Object *cursor = destination;
        unsigned int depth = 0;
        while (cursor && depth++ < 1024)
        {
            int own = 0;
            FVM_Result status =
                fvm_object_has_own_bytes(context, cursor, name.data(), name.size(), &own);
            if (status != FVM_OK || own)
                return status;
            cursor = cursor->prototype;
        }
        if (cursor)
            return FVM_ERROR;
    }
    return fvm_object_set_bytes(context, destination, name.data(), name.size(), &value);
}

/* Resolve a string-valued variable reference using its complete canonical key. */
static FVM_Result lookup_variable_value(FVM_Context *c,
                                        fvm_detail::map<fvm_detail::string, FVM_Value> *locals,
                                        FVM_Object *self, const FVM_Value &value, FVM_Value *out,
                                        bool *defined = 0)
{
    fvm_detail::string key;
    FVM_Result result = fvm_property_key(value.string, fvm_string_size(value), key);
    return result == FVM_OK ? lookup_variable(c, locals, self, key, out, defined) : result;
}

/* Execute the current value. */
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared = 0, size_t base = 0, unsigned int depth = 0,
                          bool bounded = false, FVM_Object *implicit_this = 0);
/* Execute the current value. */
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared, size_t base, unsigned int depth, bool bounded,
                          FVM_Object *implicit_this)
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
        FVM_Object *previous_this; /* [GC:] Caller identity retained by its suspended frame. */
        /* Initialize Root Guard and its owned state. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        RootGuard(FVM_Context *c, fvm_detail::vector<FVM_Value> *s,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  fvm_detail::map<fvm_detail::string, FVM_Value> *l, FVM_Object *r,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  fvm_detail::vector<FVM_Value> *reg, FVM_Object *current_this = 0)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            : context(c), previous_this(c->script_this)
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
            register_frame.receiver = current_this    ? current_this
                                      : previous_this ? previous_this
                                                      : r;
            c->script_this = register_frame.receiver;
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
            context->script_this = previous_this;
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
    } roots(c, &stack, locals, self, &registers, implicit_this);
    size_t pos = 0, pc = 0, i;
    unsigned long &steps = execution.steps;
    if (!c || (!data && size) || depth > 64)
        return FVM_INVALID_ARGUMENT;
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
            registers.resize(4);
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
            if (a.op >= 0x80)
            {
                if (size - pos < 2)
                    return FVM_INVALID_ARGUMENT;
                a.length = u16(data + pos);
                pos += 2;
                a.payload = pos;
                if (a.length > size - pos)
                    return FVM_INVALID_ARGUMENT;
                pos += a.length;
                if (a.op == 0x9b || a.op == 0x8e)
                {
                    FVM_FunctionRecord record;
                    if (!fvm_function_record(data, size, a.payload, a.length, record, a.op == 0x8e))
                        return FVM_INVALID_ARGUMENT;
                    pos = record.next;
                }
                if (a.op == 0x8f)
                {
                    FVM_TryRecord record;
                    if (!fvm_try_record(data, size, a.payload, a.length, record))
                        return FVM_INVALID_ARGUMENT;
                    pos += record.sizes[0] + record.sizes[1] + record.sizes[2];
                }
            }
            a.next = pos;
            actions.push_back(a);
            if (!a.op)
                break;
        }
        if (pos != size || (!shared && !bounded && (actions.empty() || actions.back().op != 0)))
            return FVM_INVALID_ARGUMENT;
        if (bounded && (actions.empty() || actions.back().op != 0))
        {
            /* Model the exact record boundary as a terminal action, without changing source bytes.
             */
            Action terminal;
            terminal.op = 0;
            terminal.offset = terminal.next = terminal.payload = size;
            terminal.length = 0;
            actions.push_back(terminal);
        }
        /* [GC:] Action parsing/root-frame preparation can fail without consuming finalization. */
        fvm_finalizer_enter(c);
        while (pc < actions.size())
        {
            const Action &a = actions[pc];
            fvm_debug_update_location(c, (unsigned long)(base + a.offset));
            ++steps;
            if (c->avm1_steps == std::numeric_limits<unsigned long>::max())
                return FVM_ERROR;
            ++c->avm1_steps;
            if (c->runtime->config.instruction_limit &&
                c->avm1_steps > c->runtime->config.instruction_limit)
                return FVM_ERROR;
            /* A bounded, deterministic safe point: all live AVM1 operands,
               locals and receivers are registered by RootGuard. Never collect
               in the middle of an opcode or while a native callback runs. */
            if ((steps & 63UL) == 1UL)
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                int stopped = 0;
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (c->worker && fvm_worker_stop_requested(c->worker, &stopped) == FVM_OK &&
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    stopped)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_ERROR;
                /* [GC:] Учитываем флаг активной сборки и предотвращаем повторный вход. */
                FVM_Result gc_result = c->gc_collecting ? FVM_OK : fvm_gc_step(c, 0);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (gc_result != FVM_OK)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return gc_result;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            }
            if (a.op == 0)
                return FVM_OK;
            if (a.op == 0x3e)
            { /* Return */
                if (stack.empty())
                    return FVM_ERROR;
                if (returned)
                    *returned = stack.back();
                execution.returned = true;
                return FVM_OK;
            }
            if (a.op == 0x2a)
            { /* Throw: preserve the value while unwinding. */
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Value value = stack.back();
                FVM_Result result = fvm_exception_raise(c, &value);
                stack.pop_back();
                return result;
            }
            if (a.op == 0x8f)
            {
                FVM_TryRecord record;
                if (!fvm_try_record(data, size, a.payload, a.length, record))
                    return FVM_INVALID_ARGUMENT;
                fvm_detail::vector<FVM_Value> baseline = stack;
                RootGuard baseline_roots(c, &baseline, 0, 0, &registers);
                FVM_Result result =
                    execute(c, data + record.body, record.sizes[0], returned, locals, self, 0,
                            &execution, base + record.body, depth + 1);
                /* Values saved across catch/finally need roots independent of the
                 * operand stack, which is restored during exception unwinding. */
                fvm_detail::vector<FVM_Value> saved;
                saved.resize(3);
                for (size_t n = 0; n < saved.size(); ++n)
                    fvm_value_undefined(&saved[n]);
                RootGuard saved_roots(c, &saved, 0, 0, &registers);
                if (result == FVM_EXCEPTION && (record.flags & 1))
                {
                    saved[0] = c->exception_value;
                    stack = baseline;
                    fvm_exception_clear(c);
                    fvm_detail::map<fvm_detail::string, FVM_Value> top_catch;
                    fvm_detail::map<fvm_detail::string, FVM_Value> *bindings =
                        locals ? locals : &top_catch;
                    bool had_binding = false;
                    if (record.flags & 4)
                    {
                        if (record.reg >= registers.size())
                            return FVM_INVALID_ARGUMENT;
                        registers[record.reg] = saved[0];
                    }
                    else
                    {
                        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator old =
                            bindings->find(record.name);
                        had_binding = old != bindings->end();
                        if (had_binding)
                            saved[1] = old->second;
                        (*bindings)[record.name] = saved[0];
                    }
                    result = execute(c, data + record.body + record.sizes[0], record.sizes[1],
                                     returned, bindings, self, 0, &execution,
                                     base + record.body + record.sizes[0], depth + 1);
                    if (!(record.flags & 4))
                    {
                        if (had_binding)
                            (*bindings)[record.name] = saved[1];
                        else
                            bindings->erase(record.name);
                    }
                }
                if ((record.flags & 2) && (result == FVM_OK || result == FVM_EXCEPTION))
                {
                    bool was_return = execution.returned;
                    bool was_exception = c->exception_pending;
                    bool was_fault = c->fault_pending;
                    FVM_ExceptionInfo fault = c->fault;
                    if (returned)
                        saved[1] = *returned;
                    saved[2] = c->exception_value;
                    execution.returned = false;
                    fvm_exception_clear(c);
                    size_t start = record.body + record.sizes[0] + record.sizes[1];
                    FVM_Result final_result =
                        execute(c, data + start, record.sizes[2], returned, locals, self, 0,
                                &execution, base + start, depth + 1);
                    if (final_result != FVM_OK || execution.returned)
                        result = final_result;
                    else
                    {
                        execution.returned = was_return;
                        if (returned)
                            *returned = saved[1];
                        c->exception_pending = was_exception;
                        c->exception_value = saved[2];
                        c->fault_pending = was_fault;
                        c->fault = fault;
                    }
                }
                if (result != FVM_OK || execution.returned)
                    return result;
                ++pc;
                continue;
            }
            if (a.op == 0x88)
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
            else if (a.op == 0x87)
            { /* StoreRegister leaves the operand on stack. */
                if (a.length != 1)
                    return FVM_INVALID_ARGUMENT;
                if (stack.empty())
                    return FVM_ERROR;
                unsigned int reg = data[a.payload];
                if (reg >= registers.size())
                    return FVM_INVALID_ARGUMENT;
                registers[reg] = stack.back();
            }
            else if (a.op == 0x9b || a.op == 0x8e)
            {
                FVM_FunctionRecord record;
                if (!fvm_function_record(data, size, a.payload, a.length, record, a.op == 0x8e))
                    return FVM_INVALID_ARGUMENT;
                FVM_ScriptFunction fn;
                fn.function2 = record.function2;
                fn.register_count = record.register_count;
                fn.flags = record.flags;
                fn.constants = constants;
                FVM_Result named =
                    fvm_property_key((const char *)data + record.name, record.name_size, fn.name);
                if (named != FVM_OK)
                    return named;
                size_t p = record.parameters;
                for (unsigned int parameter = 0; parameter < record.count; ++parameter)
                {

                    if (record.function2)
                        fn.parameter_registers.push_back(data[p++]);
                    size_t begin = p;
                    while (data[p])
                        ++p;
                    fvm_detail::string parameter_name;
                    named = fvm_property_key((const char *)data + begin, p - begin, parameter_name);
                    if (named != FVM_OK)
                        return named;
                    fn.parameters.push_back(parameter_name);
                    ++p;
                }
                fn.body.assign(data + record.body, data + record.body + record.size);
                FVM_AVM1Info body_info;
                body_info.struct_size = sizeof(body_info);
                if (fvm_avm1_inspect(fn.body.begin(), fn.body.size(), &body_info) != FVM_OK)

                {
                    fn.body.push_back(0);

                    if (fvm_avm1_inspect(fn.body.begin(), fn.body.size(), &body_info) != FVM_OK)
                        return FVM_INVALID_ARGUMENT;
                }
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
                    if (locals)
                        (*locals)[name] = value;
                    else
                    {
                        r = fvm_set_global_bytes(c, name.data(), name.size(), &value);
                        if (r != FVM_OK)
                            return r;
                    }
                }
                else
                    stack.push_back(value);
            }
            else if (a.op == 0x96)
            { /* Push: strings, null, undefined, boolean, double, integer */
                size_t p = a.payload, end = p + a.length;
                while (p < end)
                {
                    FVM_Value v;
                    fvm_value_undefined(&v);
                    unsigned int type = data[p++];
                    if (type == 0)
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
                    else if (type == 1)
                    {
                        if (end - p < 4)
                            return FVM_INVALID_ARGUMENT;
                        unsigned long bits = u32(data + p);
                        unsigned int exponent = (unsigned int)((bits >> 23) & 255);
                        unsigned long fraction = bits & 0x7fffffUL;
                        double number;
                        if (exponent == 255)
                            number = fraction ? std::numeric_limits<double>::quiet_NaN()
                                              : std::numeric_limits<double>::infinity();
                        else
                            number = std::ldexp((double)(fraction + (exponent ? 0x800000UL : 0)),
                                                exponent ? (int)exponent - 150 : -149);
                        fvm_value_number(&v, (bits & 0x80000000UL) ? -number : number);
                        p += 4;
                    }
                    else if (type == 4)
                    {
                        if (end - p < 1)
                            return FVM_INVALID_ARGUMENT;
                        unsigned int reg = data[p++];
                        if (reg >= registers.size())
                            return FVM_INVALID_ARGUMENT;
                        v = registers[reg];
                    }
                    else if (type == 6)
                    {
                        if (end - p < 8)
                            return FVM_INVALID_ARGUMENT;
                        /* AVM1 stores the high 32-bit word first, each word LE.
                         * Decode arithmetically to avoid alignment/endian casts. */
                        unsigned long high = u32(data + p), low = u32(data + p + 4);
                        unsigned int exponent = (unsigned int)((high >> 20) & 2047);
                        double fraction = (double)(high & 0xfffffUL) * 4294967296.0 + (double)low;
                        double number;
                        if (exponent == 2047)
                            number = fraction ? std::numeric_limits<double>::quiet_NaN()
                                              : std::numeric_limits<double>::infinity();
                        else
                            number = std::ldexp(fraction + (exponent ? 4503599627370496.0 : 0.0),
                                                exponent ? (int)exponent - 1075 : -1074);
                        fvm_value_number(&v, (high & 0x80000000UL) ? -number : number);
                        p += 8;
                    }
                    else if (type == 8 || type == 9)
                    {
                        size_t width = type == 8 ? 1 : 2;
                        if (end - p < width)
                            return FVM_INVALID_ARGUMENT;
                        unsigned int index = type == 8 ? data[p] : u16(data + p);
                        p += width;
                        if (index >= constants.size())
                            return FVM_INVALID_ARGUMENT;
                        FVM_Result result = own_string(c, constants[index].c_str(), &v);
                        if (result != FVM_OK)
                            return result;
                    }
                    else if (type == 2)
                        v.type = FVM_VALUE_NULL;
                    else if (type == 3)
                    { /* undefined */
                    }
                    else if (type == 5)
                    {
                        if (end - p < 1)
                            return FVM_INVALID_ARGUMENT;
                        fvm_value_boolean(&v, data[p++]);
                    }
                    else if (type == 7)
                    {
                        if (end - p < 4)
                            return FVM_INVALID_ARGUMENT;
                        fvm_value_number(&v, (double)((u32(data + p) & 0x80000000UL)
                                                          ? (long)(u32(data + p) & 0x7fffffffUL) -
                                                                2147483647L - 1L
                                                          : (long)u32(data + p)));
                        p += 4;
                    }
                    else
                        return FVM_UNSUPPORTED;
                    stack.push_back(v);
                }
            }
            else if (a.op == 0x42 || a.op == 0x43)
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
                double count = 0;
                if (c->swf_loaded)
                {
                    FVM_Result status = fvm_avm1_number(c, &stack.back(), &count);
                    if (status != FVM_OK)
                        return status;
                    /* Invalid signed counts consume only the count and leave operand values
                     * untouched. */
                    if (count < 0 || count > 2147483647.0)
                    {
                        fvm_value_undefined(&stack.back());
                        ++pc;
                        continue;
                    }
                    count = count == count ? std::floor(count) : 0;
                    if (count > 65536)
                        return FVM_UNSUPPORTED;
                }
                else
                {
                    if (stack.back().type != FVM_VALUE_NUMBER)
                        return FVM_ERROR;
                    count = stack.back().number;
                    if (!(count >= 0 && count <= 65536) || count != std::floor(count))
                        return FVM_INVALID_ARGUMENT;
                }
                unsigned int width = a.op == 0x42 ? 1 : 2;
                size_t available = stack.size() - 1;
                if (!c->swf_loaded && count * width > available)
                    return FVM_INVALID_ARGUMENT;
                size_t consumed = (size_t)count * width;
                if (consumed > available)
                    consumed = available;
                FVM_Object *object = 0;
                FVM_Result r = FVM_OK;
                if (a.op == 0x42)
                {
                    fvm_detail::vector<FVM_Value> values((size_t)count);
                    for (size_t j = 0; j < values.size(); ++j)
                    {
                        if (j < available)
                            values[j] = stack[stack.size() - 2 - j];
                        else
                            fvm_value_undefined(&values[j]);
                    }
                    /* Array literals never interpret a numeric element as a length. */
                    r = fvm_array_create_like(c, 0, (unsigned long)count, &object, true);
                    /* [GC:] Retain the literal result and copied elements until its own slots are
                     * published. */
                    RootGuard literal_roots(c, &values, 0, object, &registers);
                    for (size_t j = 0; r == FVM_OK && j < values.size(); ++j)
                        r = fvm_array_store_own(c, object, (unsigned long)j, values[j]);
                }
                else
                {
                    r = fvm_construct(c, "Object", 0, 0, &object);
                    /* [GC:] Retain the partially built Object and coerced names through script and
                     * setters. */
                    fvm_detail::vector<FVM_Value> temporary(3);
                    for (size_t i = 0; i < temporary.size(); ++i)
                        fvm_value_undefined(&temporary[i]);
                    RootGuard literal_roots(c, &temporary, 0, object, &registers);
                    if (r == FVM_OK)
                        for (unsigned int j = 0; j < (unsigned int)count; ++j)
                        {
                            /* Missing pair members independently pop undefined, without index
                             * underflow. */
                            if ((size_t)j * 2 < available)
                                temporary[0] = stack[stack.size() - 2 - j * 2];
                            else
                                fvm_value_undefined(&temporary[0]);
                            if ((size_t)j * 2 + 1 < available)
                                temporary[1] = stack[stack.size() - 3 - j * 2];
                            else
                                fvm_value_undefined(&temporary[1]);
                            fvm_value_undefined(&temporary[2]);
                            r = c->swf_loaded
                                    ? fvm_avm1_string(c, temporary[1], &temporary[2])
                                    : fvm_value_to_string(c, &temporary[1], &temporary[2]);
                            if (r != FVM_OK)
                                break;
                            /* AVM1 Object.set ignores an empty property name after its coercion. */
                            if (c->swf_loaded && !fvm_string_size(temporary[2]))
                                continue;
                            r = fvm_object_set_bytes(c, object, temporary[2].string,
                                                     fvm_string_size(temporary[2]), &temporary[0]);
                            if (r != FVM_OK)
                                break;
                        }
                }
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - 1 - consumed);
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = object;
                stack.push_back(value);
            }
            else if (a.op == 0x69)
            {
                /* Extends pops superclass first and does not execute either constructor. */
                if (stack.size() < 2 || stack.back().type != FVM_VALUE_OBJECT ||
                    stack[stack.size() - 2].type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                FVM_Result r =
                    fvm_class_extend(c, stack[stack.size() - 2].object, stack.back().object);
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - 2);
            }
            else if (a.op == 0x2c)
            {
                /* ImplementsOp pops the class, count, then interface constructors. */
                if (stack.size() < 2 || stack.back().type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                double count = 0;
                FVM_Result r = fvm_avm1_number(c, &stack[stack.size() - 2], &count);
                if (r != FVM_OK)
                    return r;
                if (!(count >= 0 && count <= 65536) || count != std::floor(count) ||
                    count > stack.size() - 2)
                    return FVM_INVALID_ARGUMENT;
                unsigned int n = (unsigned int)count;
                fvm_detail::vector<FVM_Value> interfaces;
                for (unsigned int i = 0; i < n; ++i)
                    interfaces.push_back(stack[stack.size() - 3 - i]);
                r = fvm_class_implements(c, stack.back().object, n, interfaces.begin());
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - n - 2);
            }
            else if (a.op == 0x2b)
            {
                /* CastOp pops the value first, then preserves it or replaces it with null. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value value = stack.back();
                int match = 0;
                FVM_Result r = fvm_value_instance_of(c, &value, &stack[stack.size() - 2], &match);
                if (r != FVM_OK)
                    return r;
                if (!match)
                {
                    fvm_value_undefined(&value);
                    value.type = FVM_VALUE_NULL;
                }
                stack.pop_back();
                stack.back() = value;
            }
            else if (a.op == 0x54)
            { /* InstanceOf: native constructor metadata and prototype identity. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                int matches = 0;
                FVM_Result result =
                    fvm_value_instance_of(c, &stack[stack.size() - 2], &stack.back(), &matches);
                if (result != FVM_OK)
                    return result;
                stack.pop_back();
                fvm_value_boolean(&stack.back(), matches);
            }
            else if (a.op == 0x04 || a.op == 0x05 || a.op == 0x06 || a.op == 0x07 || a.op == 0x81 ||
                     a.op == 0x8c || a.op == 0x9f)
            { /* Timeline controls use the selected display target, independently from script
                 methods. */
                FVM_Object *target = c->avm1_target_clip;
                bool playing = false;
                fvm_detail::vector<FVM_Value> frame(1);
                /* [GC:] Keep a coerced frame label live while resolving the target root. */
                RootGuard frame_roots(c, &frame, 0, 0, &registers);
                FVM_Result status = FVM_OK;
                if (a.op == 0x81)
                {
                    if (a.length != 2)
                        return FVM_INVALID_ARGUMENT;
                    fvm_value_number(&frame[0], u16(data + a.payload) + 1);
                }
                else if (a.op == 0x8c)
                {
                    if (!a.length || data[a.payload + a.length - 1] ||
                        std::memchr(data + a.payload, 0, a.length - 1))
                        return FVM_INVALID_ARGUMENT;
                    status = own_string(c, (const char *)data + a.payload, &frame[0], a.length - 1);
                }
                else if (a.op == 0x9f)
                {
                    if ((a.length != 1 && a.length != 3) || (data[a.payload] & ~3) ||
                        a.length != ((data[a.payload] & 2) ? 3U : 1U))
                        return FVM_INVALID_ARGUMENT;
                    if (stack.empty())
                        return FVM_ERROR;
                    playing = (data[a.payload] & 1) != 0;
                    if (stack.back().type == FVM_VALUE_NUMBER)
                        fvm_value_number(&frame[0], stack.back().number);
                    else
                        status = fvm_avm1_string(c, stack.back(), &frame[0]);
                    if (status != FVM_OK)
                        return status;
                    if (frame[0].type == FVM_VALUE_STRING)
                    {
                        bool decimal = fvm_string_size(frame[0]) != 0;
                        for (size_t digit = 0; digit < fvm_string_size(frame[0]); ++digit)
                            if (frame[0].string[digit] < '0' || frame[0].string[digit] > '9')
                                decimal = false;
                        if (decimal)
                        {
                            double number = 0;
                            status = fvm_value_to_number(c, &frame[0], &number);
                            if (status != FVM_OK)
                                return status;
                            fvm_value_number(&frame[0], number);
                        }
                    }
                    if ((data[a.payload] & 2) && frame[0].type == FVM_VALUE_NUMBER)
                        frame[0].number += u16(data + a.payload + 1);
                    status = variable_receiver(c, self, &target);
                    stack.pop_back();
                }
                else if (a.length)
                    return FVM_INVALID_ARGUMENT;
                if (status != FVM_OK)
                    return status;
                if (!target || target->resource_kind != 40)
                {
                    if (!c->avm1_base_clip)
                        return FVM_UNSUPPORTED;
                }
                else
                {
                    FVM_ExecutionFrame command = a.op == 0x04   ? FVM_EXEC_NEXT_FRAME
                                                 : a.op == 0x05 ? FVM_EXEC_PREVIOUS_FRAME
                                                 : a.op == 0x06 ? FVM_EXEC_PLAY
                                                 : a.op == 0x07 ? FVM_EXEC_STOP
                                                                : FVM_EXEC_GOTO_FRAME;
                    status = fvm_exec_frame_control(c, target, command,
                                                    a.op >= 0x80 ? &frame[0] : 0, playing);
                    if (status != FVM_OK && status != FVM_NOT_FOUND)
                        return status;
                }
            }
            else if (a.op == 0x22 || a.op == 0x23)
            { /* GetProperty/SetProperty resolve display targets after numeric index coercion. */
                const size_t count = a.op == 0x22 ? 2 : 3;
                if (stack.size() < count)
                    return FVM_ERROR;
                size_t first = stack.size() - count;
                FVM_Value result;
                FVM_Result status =
                    fvm_exec_display_property(c, a.op == 0x23, &stack[first], &stack[first + 1],
                                              a.op == 0x23 ? &stack[first + 2] : 0, &result);
                if (status != FVM_OK)
                    return status;
                stack.resize(first);
                if (a.op == 0x22)
                    stack.push_back(result);
            }
            else if (a.op == 0x8b || a.op == 0x20)
            { /* SetTarget/SetTarget2 replace the display scope without changing the this receiver.
               */
                FVM_Value path;
                if (a.op == 0x8b)
                {
                    if (!a.length || data[a.payload + a.length - 1] ||
                        std::memchr(data + a.payload, 0, a.length - 1))
                        return FVM_INVALID_ARGUMENT;
                    fvm_value_string_bytes(&path, (const char *)data + a.payload, a.length - 1);
                }
                else
                {
                    if (stack.empty())
                        return FVM_ERROR;
                    path = stack.back();
                }
                FVM_Result status = fvm_exec_select_target(c, &path);
                if (status != FVM_OK)
                    return status;
                if (a.op == 0x20)
                    stack.pop_back();
            }
            else if (a.op == 0x83 || a.op == 0x9a)
            { /* GetURL/GetURL2 delegate counted requests to an explicitly registered host sink. */
                fvm_detail::vector<FVM_Value> request(3);
                /* [GC:] The first converted target survives URL coercion that can collect. */
                RootGuard request_roots(c, &request, 0, 0, &registers);
                unsigned int flags = 0;
                FVM_Result status = FVM_OK;
                if (a.op == 0x9a)
                {
                    if (a.length != 1)
                        return FVM_INVALID_ARGUMENT;
                    flags = data[a.payload];
                    if ((flags & 0x3c) || (flags & 3) == 3)
                        return FVM_INVALID_ARGUMENT;
                    if (stack.size() < 2)
                        return FVM_ERROR;
                    request[1] = stack.back();
                    request[0] = stack[stack.size() - 2];
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
                    if (c->swf_loaded && c->swf_info.version < 6)
                        for (size_t i = 0; i < a.length; ++i)
                            if (payload[i] > 127)
                                return FVM_UNSUPPORTED;
                    status = own_string(c, (const char *)payload, &request[0], first);
                    if (status == FVM_OK)
                        status =
                            own_string(c, (const char *)separator + 1, &request[1], remaining - 1);
                }
                if (status != FVM_OK)
                    return status;
                status = fvm_exec_url(c, &request[0], &request[1], flags);
                if (status != FVM_OK)
                    return status;
                if (a.op == 0x9a)
                    stack.resize(stack.size() - 2);
            }
            else if (a.op == 0x9e)
            { /* Call executes frame actions without seeking or placing frame controls. */
                if (a.length || stack.empty())
                    return a.length ? FVM_INVALID_ARGUMENT : FVM_ERROR;
                FVM_Object *scope_receiver = 0;
                FVM_Result status = variable_receiver(c, self, &scope_receiver);
                if (status == FVM_OK)
                    status = fvm_exec_call_frame(c, scope_receiver, &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
            }
            else if (a.op == 0x40)
            { /* NewObject: separate named native constructor registry. */
                if (stack.size() < 2 || stack.back().type != FVM_VALUE_STRING ||
                    stack[stack.size() - 2].type != FVM_VALUE_NUMBER)
                    return FVM_ERROR;
                double count = stack[stack.size() - 2].number;
                if (!(count >= 0 && count <= 65536) || count != std::floor(count) ||
                    count > stack.size() - 2)
                    return FVM_ERROR;
                unsigned int argc = (unsigned int)count;
                fvm_detail::vector<FVM_Value> args(argc);
                for (unsigned int i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - 3 - i];
                FVM_Object *object = 0;
                FVM_Value constructor;
                bool defined = false;
                FVM_Result result =
                    lookup_variable_value(c, locals, self, stack.back(), &constructor, &defined);
                if (result == FVM_OK)
                    result = defined ? constructor.type == FVM_VALUE_OBJECT
                                           ? fvm_function_construct(c, constructor.object, argc,
                                                                    args.begin(), &object)
                                           : FVM_UNSUPPORTED
                                     : fvm_construct_bytes(c, stack.back().string,
                                                           fvm_string_size(stack.back()), argc,
                                                           args.begin(), &object);
                if (result != FVM_OK)
                    return result;
                stack.resize(stack.size() - argc - 2);
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = object;
                stack.push_back(value);
            }
            else if (a.op == 0x46 || a.op == 0x55)
            {
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Value target = stack.back();
                FVM_Result r = FVM_OK;
                if (a.op == 0x46)
                {
                    if (target.type != FVM_VALUE_STRING)
                        return FVM_INVALID_ARGUMENT;
                    r = lookup_variable_value(c, locals, self, target, &target);
                }
                if (r != FVM_OK)
                    return r;
                if (target.type != FVM_VALUE_OBJECT)
                    return FVM_INVALID_ARGUMENT;
                FVM_Object *keys = 0;
                r = fvm_object_enumerate(c, target.object, 1, &keys);
                if (r != FVM_OK)
                    return r;
                stack.pop_back();
                FVM_Value end;
                fvm_value_undefined(&end);
                end.type = FVM_VALUE_NULL;
                stack.push_back(end);
                for (size_t i = keys->indexed.size(); i > 0; --i)
                    stack.push_back(keys->indexed.find((unsigned long)i - 1)->second);
            }
            else if (a.op == 0x13 || a.op == 0x21 || a.op == 0x29 || a.op == 0x68)
            { /* StringEquals/StringAdd/StringLess/StringGreater use explicit string coercion. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                /* [GC:] Retain newly converted strings across collection in the second callback. */
                fvm_detail::vector<FVM_Value> converted(2);
                fvm_value_undefined(&converted[0]);
                fvm_value_undefined(&converted[1]);
                RootGuard converted_roots(c, &converted, 0, 0, &registers);
                FVM_Value &left = converted[0], &right = converted[1];
                FVM_Value output;
                FVM_Result status;
                if (a.op == 0x68)
                {
                    status = fvm_avm1_string(c, stack[stack.size() - 2], &left);
                    if (status == FVM_OK)
                        status = fvm_avm1_string(c, stack.back(), &right);
                }
                else
                {
                    status = fvm_avm1_string(c, stack.back(), &right);
                    if (status == FVM_OK)
                        status = fvm_avm1_string(c, stack[stack.size() - 2], &left);
                }
                if (status != FVM_OK)
                    return status;
                if (a.op == 0x21)
                {
                    size_t first = fvm_string_size(left), last = fvm_string_size(right);
                    if (first > (size_t)-1 - last - 1)
                        return FVM_OUT_OF_MEMORY;
                    fvm_detail::vector<char> bytes(first + last + 1);
                    std::memcpy(bytes.begin(), left.string, first);
                    std::memcpy(bytes.begin() + first, right.string, last);
                    bytes[first + last] = 0;
                    status = own_string(c, bytes.begin(), &output, first + last);
                }
                else
                {
                    int order = 0;
                    status = fvm_string_order(left, right, order);
                    fvm_value_boolean(&output, a.op == 0x13   ? order == 0
                                               : a.op == 0x29 ? order < 0
                                                              : order > 0);
                }
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = output;
            }
            else if (a.op == 0x14 || a.op == 0x31)
            { /* StringLength/MBStringLength count UTF-16 units in SWF 6+ and ASCII legacy input. */
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Value text;
                FVM_Result status = fvm_avm1_string(c, stack.back(), &text);
                if (status != FVM_OK)
                    return status;
                fvm_detail::vector<unsigned int> units;
                status = fvm_string_units(text, units);
                if (status != FVM_OK)
                    return status;
                if (c->swf_loaded && c->swf_info.version < 6)
                    for (size_t unit = 0; unit < units.size(); ++unit)
                        if (units[unit] > 127)
                            return FVM_UNSUPPORTED;
                fvm_value_number(&stack.back(), (double)units.size());
            }
            else if (a.op == 0x15 || a.op == 0x35)
            { /* StringExtract/MBStringExtract convert length, one-based start, then input text. */
                if (stack.size() < 3)
                    return FVM_ERROR;
                FVM_Value output;
                FVM_Result status = fvm_exec_string_extract(
                    c, &stack[stack.size() - 3], &stack[stack.size() - 2], &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.resize(stack.size() - 2);
                stack.back() = output;
            }
            else if (a.op == 0x32 || a.op == 0x36)
            { /* CharToAscii/MBCharToAscii return the first UTF-16 unit, replacing surrogates. */
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Value text;
                FVM_Result status = fvm_avm1_string(c, stack.back(), &text);
                if (status != FVM_OK)
                    return status;
                fvm_detail::vector<unsigned int> units;
                status = fvm_string_units(text, units);
                if (status != FVM_OK)
                    return status;
                unsigned int code = units.empty() ? 0 : units[0];
                /* Legacy byte/locale encodings remain unsupported for non-ASCII input. */
                if (c->swf_loaded && c->swf_info.version < 6 && code > 127)
                    return FVM_UNSUPPORTED;
                if (code >= 0xd800 && code <= 0xdfff)
                    code = 0xfffd;
                fvm_value_number(&stack.back(), code);
            }
            else if (a.op == 0x33 || a.op == 0x37)
            { /* AsciiToChar/MBAsciiToChar wrap to Uint16; zero is empty and surrogates are
                 replaced. */
                if (stack.empty())
                    return FVM_ERROR;
                double number = 0;
                FVM_Result status = fvm_avm1_number(c, &stack.back(), &number);
                if (status != FVM_OK)
                    return status;
                unsigned short code = (unsigned short)(integer_bits(number) & 0xffffUL);
                if (c->swf_loaded && c->swf_info.version < 6 && code > 127)
                    return FVM_UNSUPPORTED;
                if (code >= 0xd800 && code <= 0xdfff)
                    code = 0xfffd;
                FVM_Value output;
                status = fvm_value_string_utf16(c, &code, code ? 1 : 0, &output);
                if (status != FVM_OK)
                    return status;
                stack.back() = output;
            }
            else if (a.op == 0x18)
            { /* ToInteger uses signed Int32 wrapping, not an unsafe floating cast. */
                if (stack.empty())
                    return FVM_ERROR;
                double number = 0;
                FVM_Result status = fvm_avm1_number(c, &stack.back(), &number);
                if (status != FVM_OK)
                    return status;
                fvm_value_number(&stack.back(), signed_bits(integer_bits(number)));
            }
            else if (a.op == 0x44 || a.op == 0x4a || a.op == 0x4b)
            {
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Value input = stack.back(), output;
                FVM_Result r = FVM_OK;
                if (a.op == 0x44)
                {
                    const char *types[] = {"undefined", "null",   "boolean",
                                           "number",    "string", "object"};
                    const char *name = types[input.type];
                    if (input.type == FVM_VALUE_OBJECT)
                        name = input.object->invocable.callback || input.object->callable.callback
                                   ? "function"
                               : input.object->resource_kind == 40 ? "movieclip"
                                                                   : "object";
                    r = fvm_value_string(&output, name);
                }
                else if (a.op == 0x4b)
                    r = fvm_avm1_string(c, input, &output);
                else
                {
                    double number = 0;
                    r = fvm_avm1_number(c, &input, &number);
                    fvm_value_number(&output, number);
                }
                if (r != FVM_OK)
                    return r;
                stack.back() = output;
            }
            else if (a.op == 0x47 || a.op == 0x48 || a.op == 0x49 || a.op == 0x66 || a.op == 0x67)
            {
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_ExecutionCompare operation = a.op == 0x47   ? FVM_EXEC_ADD_DYNAMIC
                                                 : a.op == 0x48 ? FVM_EXEC_LESS
                                                 : a.op == 0x49 ? FVM_EXEC_EQUAL
                                                 : a.op == 0x66 ? FVM_EXEC_STRICT_EQUAL
                                                                : FVM_EXEC_GREATER;
                FVM_Value output;
                FVM_Result status = fvm_exec_compare(c, operation, &stack[stack.size() - 2],
                                                     &stack.back(), &output);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = output;
            }
            else if (a.op == 0x50 || a.op == 0x51)
            {
                if (stack.empty())
                    return FVM_ERROR;
                double value = 0;
                FVM_Result r = fvm_avm1_number(c, &stack.back(), &value);
                if (r != FVM_OK)
                    return r;
                fvm_value_number(&stack.back(), value + (a.op == 0x50 ? 1 : -1));
            }
            else if (a.op == 0x3f || (a.op >= 0x60 && a.op <= 0x65))
            {
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_ExecutionNumeric operation = a.op == 0x3f   ? FVM_EXEC_REMAINDER
                                                 : a.op == 0x60 ? FVM_EXEC_BIT_AND
                                                 : a.op == 0x61 ? FVM_EXEC_BIT_OR
                                                 : a.op == 0x62 ? FVM_EXEC_BIT_XOR
                                                 : a.op == 0x63 ? FVM_EXEC_SHIFT_LEFT
                                                 : a.op == 0x64 ? FVM_EXEC_SHIFT_RIGHT
                                                                : FVM_EXEC_SHIFT_UNSIGNED;
                FVM_Value result;
                FVM_Result status = fvm_exec_numeric(c, operation, &stack[stack.size() - 2],
                                                     &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == 0x4e || a.op == 0x4f)
            {
                size_t count = a.op == 0x4e ? 2 : 3;
                if (stack.size() < count)
                    return FVM_ERROR;
                size_t first = stack.size() - count;
                FVM_Value result;
                FVM_Result status =
                    fvm_exec_member(c, a.op == 0x4f, &stack[first], &stack[first + 1],
                                    a.op == 0x4f ? &stack[first + 2] : 0, &result);
                if (status != FVM_OK)
                    return status;
                stack.resize(first);
                if (a.op == 0x4e)
                    stack.push_back(result);
            }
            else if (a.op == 0x3a)
            { /* Delete: owner and property name; absent/inherited members report false. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                /* [GC:] Retain owner and coercible name in the stack until deletion completes. */
                FVM_Value owner = stack[stack.size() - 2], key, result;
                FVM_Result status = fvm_avm1_string(c, stack.back(), &key);
                if (status != FVM_OK)
                    return status;
                int own = 0, deleted = 0;
                if (owner.type == FVM_VALUE_OBJECT && owner.object->resource_kind != 37)
                {
                    status = fvm_object_has_own_bytes(c, owner.object, key.string,
                                                      fvm_string_size(key), &own);
                    if (status == FVM_OK && own)
                        status = fvm_object_delete_bytes(c, owner.object, key.string,
                                                         fvm_string_size(key), &deleted);
                    if (status != FVM_OK)
                        return status;
                }
                fvm_value_boolean(&result, deleted);
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == 0x4c)
            { /* PushDuplicate */
                if (stack.empty())
                    return FVM_ERROR;
                stack.push_back(stack.back());
            }
            else if (a.op == 0x4d)
            { /* StackSwap */
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value tmp = stack.back();
                stack.back() = stack[stack.size() - 2];
                stack[stack.size() - 2] = tmp;
            }
            else if (a.op == 0x12)
            { /* Not */
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Value &v = stack.back();
                bool converted = false;
                FVM_Result status = action_truth(c, v, converted);
                if (status != FVM_OK)
                    return status;
                fvm_value_boolean(&v, !converted);
            }
            else if (a.op == 0x0e || a.op == 0x0f || a.op == 0x10 || a.op == 0x11)
            {
                /* [GC:] Keep both operands in the rooted stack through native coercion. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value operands[2] = {stack[stack.size() - 2], stack.back()}, result;
                bool value = false;
                if (a.op == 0x0e || a.op == 0x0f)
                {
                    const char *hook = a.op == 0x0e ? "__eq" : "__lt";
                    FVM_Result status =
                        fvm_value_meta_invoke(c, operands, hook, 2, operands, &result);
                    if (status == FVM_NOT_FOUND)
                        status = fvm_value_meta_invoke(c, operands + 1, hook, 2, operands, &result);
                    if (status == FVM_OK)
                        value = truth(result);
                    else if (status != FVM_NOT_FOUND)
                        return status;
                    else
                    {
                        double left = 0, right = 0;
                        /* Legacy Equals converts the top operand first; Less converts left first.
                         */
                        if (a.op == 0x0e)
                        {
                            status = fvm_avm1_number(c, operands + 1, &right);
                            if (status == FVM_OK)
                                status = fvm_avm1_number(c, operands, &left);
                        }
                        else
                        {
                            status = fvm_avm1_number(c, operands, &left);
                            if (status == FVM_OK)
                                status = fvm_avm1_number(c, operands + 1, &right);
                        }
                        if (status != FVM_OK)
                            return status;
                        value = a.op == 0x0e ? left == right : left < right;
                    }
                }
                else
                {
                    bool left = false, right = false;
                    FVM_Result status = action_truth(c, operands[0], left);
                    if (status == FVM_OK)
                        status = action_truth(c, operands[1], right);
                    if (status != FVM_OK)
                        return status;
                    value = a.op == 0x10 ? left && right : left || right;
                }
                fvm_value_boolean(&result, value);
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == 0x26)
            { /* Trace always coerces its operand, even when host delivery is disabled. */
                if (stack.empty())
                    return FVM_ERROR;
                FVM_Result status = fvm_exec_trace(c, &stack.back());
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
            }
            else if (a.op == 0x17)
            { /* Pop */
                if (stack.empty())
                    return FVM_ERROR;
                stack.pop_back();
            }
            else if (a.op == 0x3c || a.op == 0x41)
            { /* DefineLocal / DeclareLocal also bind top-level clip/global properties. */
                if (stack.size() < (a.op == 0x3c ? 2U : 1U))
                    return FVM_ERROR;
                /* [GC:] Retain the name and initialized value through coercion/setters. */
                const FVM_Value &name_value =
                    stack[a.op == 0x3c ? stack.size() - 2 : stack.size() - 1];
                FVM_Value text;
                FVM_Result status = fvm_avm1_string(c, name_value, &text);
                if (status != FVM_OK)
                    return status;
                fvm_detail::string name;
                status = fvm_property_key(text.string, fvm_string_size(text), name);
                if (status != FVM_OK)
                    return status;
                FVM_Value value;
                fvm_value_undefined(&value);
                if (a.op == 0x3c)
                    value = stack.back();
                status = declare_variable(c, locals, self, name, value, a.op == 0x3c);
                if (status != FVM_OK)
                    return status;
                stack.resize(stack.size() - (a.op == 0x3c ? 2 : 1));
            }
            else if (a.op == 0x1c)
            {
                if (stack.empty() || stack.back().type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                FVM_Value value;
                FVM_Result r = lookup_variable_value(c, locals, self, stack.back(), &value);
                if (r != FVM_OK)
                    return r;
                stack.back() = value;
            }
            else if (a.op == 0x1d)
            { /* SetVariable: name, value */
                if (stack.size() < 2 || stack[stack.size() - 2].type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                const FVM_Value &name_value = stack[stack.size() - 2];
                fvm_detail::string name;
                FVM_Result normalized =
                    fvm_property_key(name_value.string, fvm_string_size(name_value), name);
                if (normalized != FVM_OK)
                    return normalized;
                FVM_Result r = FVM_OK;
                if (locals && locals->find(name) != locals->end())
                    (*locals)[name] = stack.back();
                else
                {
                    FVM_Object *captured =
                        c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back()->prototype;
                    unsigned int levels = 0;
                    while (captured && levels++ < 1024 &&
                           captured->properties.find(name) == captured->properties.end())
                        captured = captured->prototype;
                    if (captured && levels > 1024)
                        return FVM_ERROR;
                    FVM_Object *scope_receiver = 0;
                    r = variable_receiver(c, self, &scope_receiver);
                    if (r == FVM_OK)
                        r = captured ? fvm_object_set_bytes(c, captured, name.data(), name.size(),
                                                            &stack.back())
                            : scope_receiver && scope_receiver->resource_kind == 40
                                ? fvm_object_set_bytes(c, scope_receiver, name.data(), name.size(),
                                                       &stack.back())
                                : fvm_set_global_bytes(c, name.data(), name.size(), &stack.back());
                }
                if (r != FVM_OK)
                    return r;
                stack.pop_back();
                stack.pop_back();
            }
            else if (a.op == 0x0a || a.op == 0x0b || a.op == 0x0c || a.op == 0x0d)
            {
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_ExecutionNumeric operation = a.op == 0x0a   ? FVM_EXEC_ADD
                                                 : a.op == 0x0b ? FVM_EXEC_SUBTRACT
                                                 : a.op == 0x0c ? FVM_EXEC_MULTIPLY
                                                                : FVM_EXEC_DIVIDE;
                FVM_Value result;
                FVM_Result status = fvm_exec_numeric(c, operation, &stack[stack.size() - 2],
                                                     &stack.back(), &result);
                if (status != FVM_OK)
                    return status;
                stack.pop_back();
                stack.back() = result;
            }
            else if (a.op == 0x52 || a.op == 0x53)
            { /* CallMethod: args, argc, receiver, method name */
                if (stack.size() < 3 ||
                    (stack.back().type != FVM_VALUE_STRING && stack.back().type != FVM_VALUE_NULL &&
                     stack.back().type != FVM_VALUE_UNDEFINED) ||
                    stack[stack.size() - 3].type != FVM_VALUE_NUMBER)
                    return FVM_ERROR;
                double n = stack[stack.size() - 3].number;
                if (!(n >= 0 && n <= 65535) || n != std::floor(n) || stack.size() < (size_t)n + 3)
                    return FVM_ERROR;
                FVM_Value owner = stack[stack.size() - 2];
                if (a.op == 0x52 &&
                    (owner.type == FVM_VALUE_NULL || owner.type == FVM_VALUE_UNDEFINED))
                {
                    /* A nullish receiver consumes valid arguments without invoking a method. */
                    stack.resize(stack.size() - (size_t)n - 3);
                    FVM_Value missing;
                    fvm_value_undefined(&missing);
                    stack.push_back(missing);
                    ++pc;
                    continue;
                }
                fvm_detail::string name;
                if (stack.back().type == FVM_VALUE_STRING)
                {
                    FVM_Result normalized =
                        fvm_property_key(stack.back().string, fvm_string_size(stack.back()), name);
                    if (normalized != FVM_OK)
                        return normalized;
                }

                FVM_Object *receiver = owner.type == FVM_VALUE_OBJECT ? owner.object : 0;
                if (!receiver)
                {
                    if (owner.type == FVM_VALUE_NULL || owner.type == FVM_VALUE_UNDEFINED)
                        return FVM_INVALID_ARGUMENT;
                    const char *type = owner.type == FVM_VALUE_STRING    ? "String"
                                       : owner.type == FVM_VALUE_BOOLEAN ? "Boolean"
                                                                         : "Number";
                    FVM_Result boxed = fvm_construct(c, type, 1, &owner, &receiver);
                    if (boxed != FVM_OK)
                        return boxed;
                }

                unsigned int argc = (unsigned int)n;
                fvm_detail::vector<FVM_Value> args(argc);
                for (i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - 4 - i];
                FVM_Value result;
                fvm_value_undefined(&result);
                FVM_Result r;
                fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator script =
                    c->script_functions.find(name);
                FVM_NativeEntry native_method;
                FVM_Result lookup = fvm_lookup_native_method(receiver, name, &native_method);
                if (lookup != FVM_OK && lookup != FVM_NOT_FOUND && lookup != FVM_UNSUPPORTED)
                    return lookup;
                if (a.op == 0x53)
                {
                    FVM_Value function;
                    r = name.empty() ? FVM_OK
                                     : fvm_object_get_bytes(c, receiver, name.data(), name.size(),
                                                            &function);
                    if (name.empty())
                    {
                        fvm_value_undefined(&function);
                        function.type = FVM_VALUE_OBJECT;
                        function.object = receiver;
                    }
                    if (r != FVM_OK)
                        return r;
                    if (function.type != FVM_VALUE_OBJECT)
                        return FVM_INVALID_ARGUMENT;
                    FVM_Object *instance = 0;
                    r = fvm_function_construct(c, function.object, argc, args.begin(), &instance);
                    if (r == FVM_OK)
                    {
                        result.type = FVM_VALUE_OBJECT;
                        result.object = instance;
                    }
                }
                else if (name.empty())
                    r = fvm_function_invoke(c, receiver, self, argc, args.begin(), &result);
                else if (receiver->resource_kind != 37 && lookup == FVM_NOT_FOUND &&
                         script != c->script_functions.end())
                    r = fvm_avm1_call_script(c, script->first, script->second, argc,
                                             argc ? &args[0] : 0, receiver, &result);
                else
                    r = fvm_object_call_bytes(c, receiver, name.data(), name.size(), argc,
                                              argc ? &args[0] : 0, &result);
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - argc - 3);
                stack.push_back(result);
            }
            else if (a.op == 0x3d)
            { /* CallFunction: args, argc, name (top) */
                if (stack.size() < 2 || stack.back().type != FVM_VALUE_STRING ||
                    stack[stack.size() - 2].type != FVM_VALUE_NUMBER)
                    return FVM_ERROR;
                fvm_detail::string name;
                FVM_Result normalized =
                    fvm_property_key(stack.back().string, fvm_string_size(stack.back()), name);
                if (normalized != FVM_OK)
                    return normalized;
                double n = stack[stack.size() - 2].number;
                if (!(n >= 0 && n <= 65535) || n != std::floor(n) || stack.size() < (size_t)n + 2)
                    return FVM_ERROR;
                unsigned int argc = (unsigned int)n;
                fvm_detail::vector<FVM_Value> args(argc);
                for (i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - 3 - i];
                FVM_Value result;
                FVM_Result r;
                fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator fn =
                    c->script_functions.find(name);
                FVM_Value callable;
                fvm_value_undefined(&callable);
                bool defined = false;
                FVM_Object *call_receiver = c->global;
                r = lookup_variable(c, locals, self, name, &callable, &defined, &call_receiver);
                if (r != FVM_OK)
                    return r;
                if (callable.type == FVM_VALUE_OBJECT && callable.object->invocable.callback)
                    r = fvm_function_invoke(c, callable.object, call_receiver, argc, args.begin(),
                                            &result);
                else if (defined)
                    r = FVM_UNSUPPORTED;
                else if (fn != c->script_functions.end())
                    r = fvm_avm1_call_script(c, fn->first, fn->second, argc, args.begin(),
                                             c->global, &result);
                else
                    r = fvm_call_bytes(c, name.data(), name.size(), argc, args.begin(), &result);
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - argc - 2);
                stack.push_back(result);
            }
            else if (a.op == 0x99 || a.op == 0x9d)
            { /* Jump / If */
                if (a.length != 2)
                    return FVM_INVALID_ARGUMENT;
                int take = 1;
                if (a.op == 0x9d)
                {
                    if (stack.empty())
                        return FVM_ERROR;
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
                    if (delta >= 32768)
                        delta -= 65536;
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
    return execute(c, (const unsigned char *)data, size, 0, 0, c->global, 0, 0, 0, 0, c->swf_loaded,
                   c->global);
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
    size_t rect = (5 + 4 * (p[8] >> 3) + 7) / 8;
    size_t pos = 8 + rect + 4;
    while (pos < size)
    {
        unsigned int header = u16(p + pos);
        pos += 2;
        unsigned int code = header >> 6;
        size_t len = header & 63;
        if (len == 63)
        {
            len = (size_t)u32(p + pos);
            pos += 4;
        }
        if (code == 12)
        {
            FVM_Result r = fvm_avm1_execute(c, p + pos, len);
            if (r != FVM_OK)
                return r;
        }
        pos += len;
        if (!code)
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
                                       FVM_Object *implicit_this)
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
    return execute(c, (const unsigned char *)data, size, 0, 0, receiver, 0, 0, 0, 0, c->swf_loaded,
                   receiver);
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
    return result == FVM_OK ? execute(c, (const unsigned char *)data, size, 0, 0, receiver, 0, 0, 0,
                                      0, true, receiver)
                            : result;
}
