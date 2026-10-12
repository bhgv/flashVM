#include "function_record.hpp"
#include "../standard/function_state.hpp"
#include "../standard/base_state.hpp"
#include "../standard/primitive_state.hpp"
#include "flashvm/function.h"
#include "../standard/string_order.hpp"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include "try_record.hpp"
#include "../core/exception_internal.hpp"
#include "../core/runtime.hpp"
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
struct Action
{
    unsigned char op;
    size_t offset, next, payload, length;
};
static unsigned int u16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}
static unsigned long u32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}
/* Bytecode buffers and function snapshots can disappear after execution.
 * Any string escaping into VM properties must have context lifetime. */
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

static FVM_Result own_string(FVM_Context *context, const char *text, FVM_Value *value,
                             size_t length = (size_t)-1)
{
    if (length == (size_t)-1)
        length = std::strlen(text);
    char *copy = new (std::nothrow) char[length + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;
    std::memcpy(copy, text, length);
    copy[length] = 0;
    try
    {
        context->strings.push_back(copy);
    }
    catch (...)
    {
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    return fvm_value_string_bytes(value, copy, length);
}
static bool date_hint(const FVM_Value &value)
{
    return value.type == FVM_VALUE_OBJECT && value.object->resource_kind == 34;
}
static bool strict_equal(const FVM_Value &a, const FVM_Value &b)
{
    if (a.type != b.type)
        return false;
    switch (a.type)
    {
    case FVM_VALUE_UNDEFINED:
    case FVM_VALUE_NULL:
        return true;
    case FVM_VALUE_BOOLEAN:
        return !!a.boolean == !!b.boolean;
    case FVM_VALUE_NUMBER:
        return a.number == b.number;
    case FVM_VALUE_STRING:
    {
        int order = 0;
        return fvm_string_order(a, b, order) == FVM_OK && order == 0;
    }
    case FVM_VALUE_OBJECT:
        return a.object == b.object;
    }
    return false;
}
static FVM_Result equal(FVM_Context *c, FVM_Value a, FVM_Value b, bool &result,
                        unsigned int depth = 0)
{
    if (depth > 8)
        return FVM_ERROR;
    if (a.type == b.type)
    {
        result = strict_equal(a, b);
        return FVM_OK;
    }
    if ((a.type == FVM_VALUE_NULL && b.type == FVM_VALUE_UNDEFINED) ||
        (b.type == FVM_VALUE_NULL && a.type == FVM_VALUE_UNDEFINED))
    {
        result = true;
        return FVM_OK;
    }
    if (a.type == FVM_VALUE_BOOLEAN)
    {
        fvm_value_number(&a, a.boolean ? 1 : 0);
        return equal(c, a, b, result, depth + 1);
    }
    if (b.type == FVM_VALUE_BOOLEAN)
    {
        fvm_value_number(&b, b.boolean ? 1 : 0);
        return equal(c, a, b, result, depth + 1);
    }
    if ((a.type == FVM_VALUE_NUMBER && b.type == FVM_VALUE_STRING) ||
        (b.type == FVM_VALUE_NUMBER && a.type == FVM_VALUE_STRING))
    {
        double x = 0, y = 0;
        FVM_Result r = fvm_value_to_number(c, &a, &x);
        if (r == FVM_OK)
            r = fvm_value_to_number(c, &b, &y);
        result = x == y;
        return r;
    }
    if (a.type == FVM_VALUE_OBJECT && (b.type == FVM_VALUE_STRING || b.type == FVM_VALUE_NUMBER))
    {
        FVM_Result r = fvm_to_primitive(c, &a, date_hint(a), &a);
        return r == FVM_OK ? equal(c, a, b, result, depth + 1) : r;
    }
    if (b.type == FVM_VALUE_OBJECT && (a.type == FVM_VALUE_STRING || a.type == FVM_VALUE_NUMBER))
    {
        FVM_Result r = fvm_to_primitive(c, &b, date_hint(b), &b);
        return r == FVM_OK ? equal(c, a, b, result, depth + 1) : r;
    }
    result = false;
    return FVM_OK;
}
static FVM_Result lookup_variable(FVM_Context *c,
                                  fvm_detail::map<fvm_detail::string, FVM_Value> *locals,
                                  FVM_Object *self, const char *name, FVM_Value *out,
                                  bool *defined = 0, FVM_Object **call_receiver = 0)
{
    if (defined)
        *defined = true;
    if (call_receiver)
        *call_receiver = c->global;
    if (locals)
    {
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator local = locals->find(name);
        if (local != locals->end())
        {
            *out = local->second;
            return FVM_OK;
        }
    }
    if (self && !std::strcmp(name, "this"))
    {
        fvm_value_undefined(out);
        out->type = FVM_VALUE_OBJECT;
        out->object = self;
        return FVM_OK;
    }
    FVM_Object *binding = c->lexical_scopes.empty() ? 0 : c->lexical_scopes.back();
    unsigned int depth = 0;
    while (binding && depth++ < 1024)
    {
        if (binding->properties.find(name) != binding->properties.end())
            return fvm_object_get(c, binding, name, out);
        binding = binding->prototype;
    }
    if (binding)
        return FVM_ERROR;
    binding = self && self != c->global ? self : 0;
    depth = 0;
    while (binding && depth++ < 1024)
    {
        if (binding->properties.find(name) != binding->properties.end() ||
            binding->methods.find(name) != binding->methods.end() ||
            binding->accessors.find(name) != binding->accessors.end())
        {
            if (call_receiver)
                *call_receiver = self;
            return fvm_object_get(c, self, name, out);
        }
        binding = binding->prototype;
    }
    if (binding)
        return FVM_ERROR;
    FVM_Result r = fvm_get_global(c, name, out);
    if (r == FVM_OK && defined && out->type == FVM_VALUE_UNDEFINED)
    {
        int own = 0;
        r = fvm_object_has_own(c, c->global, name, &own);
        *defined = own != 0;
    }
    return r;
}
struct Execution
{
    fvm_detail::vector<fvm_detail::string> constants;
    fvm_detail::vector<FVM_Value> registers, stack;
    unsigned long steps;
    bool returned;
    Execution() : steps(0), returned(false)
    {
    }
};
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared = 0, size_t base = 0, unsigned int depth = 0);
/* A call frame owns parameter bindings and its receiver; script depth is restored
 * even when the interpreter reports an error. */
static FVM_Result call_script(FVM_Context *c, const char *name, const FVM_ScriptFunction &fn,
                              unsigned int argc, const FVM_Value *args, FVM_Object *receiver,
                              FVM_Value *result, FVM_Object *closure = 0, FVM_Object *callee = 0)
{
    if (c->script_depth >= 64)
        return FVM_ERROR;
    FVM_Object *activation = 0;
    FVM_Result activation_result = fvm_object_create(c, &activation);
    if (activation_result != FVM_OK)
        return activation_result;
    fvm_detail::map<fvm_detail::string, FVM_Value> &frame = activation->properties;
    try
    {
        FVM_Object *arguments = 0;
        FVM_Result stored = fvm_object_create(c, &arguments);
        if (stored != FVM_OK)
            return stored;
        arguments->resource_kind = 30;
        FVM_Value value;
        fvm_value_number(&value, argc);
        stored = fvm_object_set(c, arguments, "length", &value);
        if (stored != FVM_OK)
            return stored;
        arguments->attributes["length"] = 1;
        for (unsigned int i = 0; i < argc; ++i)
        {
            stored = fvm_table_set_index(c, arguments, i, args + i);
            if (stored != FVM_OK)
                return stored;
        }
        if (callee)
        {
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = callee;
            stored = fvm_object_set(c, arguments, "callee", &value);
            if (stored != FVM_OK)
                return stored;
            arguments->attributes["callee"] = 1;
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = arguments;
        stored = fvm_object_set(c, activation, "arguments", &value);
        if (stored != FVM_OK)
            return stored;
        /* Explicit parameters can shadow the implicit arguments binding. */
        for (size_t i = 0; i < fn.parameters.size(); ++i)
        {
            FVM_Value value;
            fvm_value_undefined(&value);
            if (i < argc)
                value = args[i];
            FVM_Result stored = fvm_object_set(c, activation, fn.parameters[i].c_str(), &value);
            if (stored != FVM_OK)
                return stored;
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
    struct DebugFrame
    {
        FVM_Context *context;
        fvm_detail::string function, source;
        unsigned long line, offset;
        unsigned int depth;
        explicit DebugFrame(FVM_Context *c)
            : context(c), function(c->debug_function), source(c->debug_source), line(c->debug_line),
              offset(c->debug_offset), depth(c->script_depth)
        {
        }
        ~DebugFrame()
        {
            context->debug_function.swap(function);
            context->debug_source.swap(source);
            context->debug_line = line;
            context->debug_offset = offset;
            context->script_depth = depth;
        }
    };
    FVM_Result status;
    try
    {
        DebugFrame debug(c);
        c->debug_function = name;
        c->debug_line = 0;
        ++c->script_depth;
        activation->prototype = closure;
        c->lexical_scopes.push_back(activation);
        struct PopScope
        {
            FVM_Context *c;
            explicit PopScope(FVM_Context *context) : c(context)
            {
            }
            ~PopScope()
            {
                c->lexical_scopes.pop_back();
            }
        } scope(c);
        status = execute(c, &snapshot.body[0], snapshot.body.size(), result, &frame, receiver,
                         &snapshot.constants);
        --c->script_depth;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
    /* A returned bytecode literal must outlive the function snapshot. */
    if (status == FVM_OK && result && result->type == FVM_VALUE_STRING)
    {
        size_t len = fvm_string_size(*result);
        char *copy = new (std::nothrow) char[len + 1];
        if (!copy)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(copy, result->string, len);
        copy[len] = 0;
        try
        {
            c->strings.push_back(copy);
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
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared, size_t base, unsigned int depth)
{
    /* Nested functions and native reentry consume the same outer instruction
     * budget. Separate operand stacks must not reset this security boundary. */
    struct BudgetGuard
    {
        FVM_Context *context;
        explicit BudgetGuard(FVM_Context *c) : context(c)
        {
            if (!c->avm1_execution_depth)
                c->avm1_steps = 0;
            ++c->avm1_execution_depth;
        }
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
    struct RootGuard
    {
        FVM_Context *context;
        FVM_GCRootFrame frame, register_frame;
        RootGuard(FVM_Context *c, fvm_detail::vector<FVM_Value> *s,
                  fvm_detail::map<fvm_detail::string, FVM_Value> *l, FVM_Object *r,
                  fvm_detail::vector<FVM_Value> *reg)
            : context(c)
        {
            frame.previous = c->gc_frames;
            frame.stack = s;
            frame.locals = l;
            frame.receiver = r;
            register_frame.previous = &frame;
            register_frame.stack = reg;
            register_frame.locals = 0;
            register_frame.receiver = 0;
            c->gc_frames = &register_frame;
        }
        ~RootGuard()
        {
            context->gc_frames = frame.previous;
        }
    } roots(c, &stack, locals, self, &registers);
    size_t pos = 0, pc = 0, i;
    unsigned long &steps = execution.steps;
    if (!c || (!data && size) || depth > 64)
        return FVM_INVALID_ARGUMENT;
    if (returned && !shared)
        fvm_value_undefined(returned);
    try
    {
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
                if (a.op == 0x9b)
                {
                    FVM_FunctionRecord record;
                    if (!fvm_function_record(data, size, a.payload, a.length, record))
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
        if ((!shared && (actions.empty() || actions.back().op != 0)) || pos != size)
            return FVM_INVALID_ARGUMENT;
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
            {
                int stopped = 0;
                if (c->worker && fvm_worker_stop_requested(c->worker, &stopped) == FVM_OK &&
                    stopped)
                    return FVM_ERROR;
                FVM_Result gc_result = c->gc_collecting ? FVM_OK : fvm_gc_step(c, 0);
                if (gc_result != FVM_OK)
                    return gc_result;
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
            else if (a.op == 0x9b)
            {
                FVM_FunctionRecord record;
                if (!fvm_function_record(data, size, a.payload, a.length, record))
                    return FVM_INVALID_ARGUMENT;
                FVM_ScriptFunction fn;
                fn.constants = constants;
                fn.name = fvm_detail::string((const char *)data + record.name, record.name_size);
                size_t p = record.parameters;
                for (unsigned int parameter = 0; parameter < record.count; ++parameter)
                {
                    size_t begin = p;
                    while (data[p])
                        ++p;
                    fn.parameters.push_back(
                        fvm_detail::string((const char *)data + begin, p - begin));
                    ++p;
                }
                fn.body.assign(data + record.body, data + record.body + record.size);
                FVM_AVM1Info body_info;
                body_info.struct_size = sizeof(body_info);
                if (fvm_avm1_inspect(fn.body.begin(), fn.body.size(), &body_info) != FVM_OK)
                    fn.body.push_back(0);
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
                    fvm_detail::string name((const char *)data + record.name, record.name_size);
                    c->script_functions[name] = fn;
                    if (locals)
                        (*locals)[name] = value;
                    else
                    {
                        r = fvm_set_global(c, name.c_str(), &value);
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
                if (stack.empty() || stack.back().type != FVM_VALUE_NUMBER)
                    return FVM_ERROR;
                double count = stack.back().number;
                unsigned int width = a.op == 0x42 ? 1 : 2;
                if (!(count >= 0 && count <= 65536) || count != std::floor(count) ||
                    count * width > stack.size() - 1)
                    return FVM_INVALID_ARGUMENT;
                FVM_Object *object = 0;
                FVM_Result r = FVM_OK;
                if (a.op == 0x42)
                {
                    fvm_detail::vector<FVM_Value> values((size_t)count);
                    for (size_t j = 0; j < values.size(); ++j)
                        values[j] = stack[stack.size() - 2 - j];
                    /* Array literals never interpret a numeric element as a length. */
                    r = fvm_array_create_like(c, 0, (unsigned long)count, &object);
                    for (size_t j = 0; r == FVM_OK && j < values.size(); ++j)
                        r = fvm_table_set_index(c, object, (unsigned long)j, &values[j]);
                }
                else
                {
                    r = fvm_construct(c, "Object", 0, 0, &object);
                    if (r == FVM_OK)
                        for (unsigned int j = 0; j < (unsigned int)count; ++j)
                        {
                            FVM_Value value = stack[stack.size() - 2 - j * 2],
                                      key = stack[stack.size() - 3 - j * 2], text;
                            r = fvm_value_to_string(c, &key, &text);
                            if (r != FVM_OK)
                                break;
                            r = fvm_object_set(c, object, text.string, &value);
                            if (r != FVM_OK)
                                break;
                        }
                }
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - 1 - (size_t)count * width);
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = object;
                stack.push_back(value);
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
            {
                if (!self || self->resource_kind != 40)
                    return FVM_UNSUPPORTED;
                const char *method = a.op == 0x04   ? "nextFrame"
                                     : a.op == 0x05 ? "prevFrame"
                                     : a.op == 0x06 ? "play"
                                                    : "stop";
                FVM_Value frame, result;
                unsigned int count = 0;
                if (a.op == 0x81 || a.op == 0x8c || a.op == 0x9f)
                {
                    count = 1;
                    bool playing = false;
                    FVM_Object *state = self->resource_result.object;
                    if (!self->resource_result_valid ||
                        self->resource_result.type != FVM_VALUE_OBJECT || !state)
                        return FVM_INVALID_ARGUMENT;
                    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator play =
                        state->properties.find("playing");
                    if (play != state->properties.end())
                        playing = play->second.number != 0;
                    if (a.op == 0x81)
                    {
                        if (a.length != 2)
                            return FVM_INVALID_ARGUMENT;
                        fvm_value_number(&frame, u16(data + a.payload) + 1);
                    }
                    else if (a.op == 0x8c)
                    {
                        if (!a.length || !std::memchr(data + a.payload, 0, a.length))
                            return FVM_INVALID_ARGUMENT;
                        FVM_Result r = own_string(c, (const char *)data + a.payload, &frame);
                        if (r != FVM_OK)
                            return r;
                    }
                    else
                    {
                        if ((a.length != 1 && a.length != 3) || stack.empty() ||
                            (data[a.payload] & ~3) || a.length != ((data[a.payload] & 2) ? 3U : 1U))
                            return FVM_INVALID_ARGUMENT;
                        frame = stack.back();
                        stack.pop_back();
                        playing = (data[a.payload] & 1) != 0;
                        if ((data[a.payload] & 2) && frame.type == FVM_VALUE_NUMBER)
                            frame.number += u16(data + a.payload + 1);
                    }
                    method = playing ? "gotoAndPlay" : "gotoAndStop";
                }
                FVM_Result r = fvm_object_call(c, self, method, count, count ? &frame : 0, &result);
                if (r != FVM_OK)
                    return r;
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
                    lookup_variable(c, locals, self, stack.back().string, &constructor, &defined);
                if (result == FVM_OK)
                    result = defined ? constructor.type == FVM_VALUE_OBJECT
                                           ? fvm_function_construct(c, constructor.object, argc,
                                                                    args.begin(), &object)
                                           : FVM_UNSUPPORTED
                                     : fvm_construct(c, stack.back().string, argc, args.begin(),
                                                     &object);
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
                    r = lookup_variable(c, locals, self, target.string, &target);
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
                    r = fvm_value_to_string(c, &input, &output);
                else
                {
                    double number = 0;
                    r = fvm_value_to_number(c, &input, &number);
                    fvm_value_number(&output, number);
                }
                if (r != FVM_OK)
                    return r;
                stack.back() = output;
            }
            else if (a.op == 0x47 || a.op == 0x48 || a.op == 0x49 || a.op == 0x66)
            {
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value left = stack[stack.size() - 2], right = stack.back(), output;
                FVM_Result r = FVM_OK;
                if (a.op != 0x66)
                {
                    const char *hook = a.op == 0x47 ? "__add" : a.op == 0x48 ? "__lt" : "__eq";
                    FVM_Value operands[2] = {left, right};
                    r = fvm_value_meta_invoke(c, operands, hook, 2, operands, &output);
                    if (r == FVM_NOT_FOUND)
                        r = fvm_value_meta_invoke(c, operands + 1, hook, 2, operands, &output);
                    if (r == FVM_OK)
                    {
                        if (a.op != 0x47)
                            fvm_value_boolean(&output, truth(output));
                        stack.pop_back();
                        stack.back() = output;
                        ++pc;
                        continue;
                    }
                    if (r != FVM_NOT_FOUND)
                        return r;
                    r = FVM_OK;
                }
                if (a.op == 0x66 || a.op == 0x49)
                {
                    bool matches = false;
                    r = a.op == 0x66 ? FVM_OK : equal(c, left, right, matches);
                    if (a.op == 0x66)
                        matches = strict_equal(left, right);
                    fvm_value_boolean(&output, matches);
                }
                else
                {
                    r = fvm_to_primitive(c, &left, a.op == 0x47 && date_hint(left), &left);
                    if (r == FVM_OK)
                        r = fvm_to_primitive(c, &right, a.op == 0x47 && date_hint(right), &right);
                    if (r == FVM_OK && a.op == 0x47 &&
                        (left.type == FVM_VALUE_STRING || right.type == FVM_VALUE_STRING))
                    {
                        FVM_Value x, y;
                        r = fvm_value_to_string(c, &left, &x);
                        if (r == FVM_OK)
                            r = fvm_value_to_string(c, &right, &y);
                        if (r == FVM_OK)
                        {
                            size_t first = fvm_string_size(x), last = fvm_string_size(y);
                            if (first > (size_t)-1 - last - 1)
                                return FVM_OUT_OF_MEMORY;
                            fvm_detail::vector<char> text(first + last + 1);
                            std::memcpy(text.begin(), x.string, first);
                            std::memcpy(text.begin() + first, y.string, last);
                            text[first + last] = 0;
                            r = own_string(c, text.begin(), &output, first + last);
                        }
                    }
                    else if (r == FVM_OK && a.op == 0x48 && left.type == FVM_VALUE_STRING &&
                             right.type == FVM_VALUE_STRING)
                    {
                        int order = 0;
                        r = fvm_string_order(left, right, order);
                        fvm_value_boolean(&output, order < 0);
                    }
                    else if (r == FVM_OK)
                    {
                        double x = 0, y = 0;
                        r = fvm_value_to_number(c, &left, &x);
                        if (r == FVM_OK)
                            r = fvm_value_to_number(c, &right, &y);
                        if (a.op == 0x47)
                            fvm_value_number(&output, x + y);
                        else
                            fvm_value_boolean(&output, x < y);
                    }
                }
                if (r != FVM_OK)
                    return r;
                stack.pop_back();
                stack.back() = output;
            }
            else if (a.op == 0x4e)
            {
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value key, value;
                const FVM_Value &owner = stack[stack.size() - 2];
                const FVM_Value &member = stack.back();
                FVM_Result r;
                /* Legacy native tables expose numeric slots through the indexed API. */
                if (owner.type == FVM_VALUE_OBJECT && member.type == FVM_VALUE_NUMBER &&
                    member.number >= 0 && member.number <= 4294967295.0 &&
                    member.number == std::floor(member.number))
                    r = fvm_table_get_index(c, owner.object, (unsigned long)member.number, &value);
                else
                {
                    r = fvm_value_to_string(c, &member, &key);
                    if (r == FVM_OK)
                        r = fvm_value_get_property(c, &owner, key.string, &value);
                }
                if (r != FVM_OK)
                    return r;
                stack.pop_back();
                stack.back() = value;
            }
            else if (a.op == 0x4f)
            {
                if (stack.size() < 3)
                    return FVM_ERROR;
                FVM_Value key;
                const FVM_Value &owner = stack[stack.size() - 3];
                const FVM_Value &member = stack[stack.size() - 2];
                FVM_Result r;
                if (owner.type == FVM_VALUE_OBJECT && member.type == FVM_VALUE_NUMBER &&
                    member.number >= 0 && member.number <= 4294967295.0 &&
                    member.number == std::floor(member.number))
                    r = fvm_table_set_index(c, owner.object, (unsigned long)member.number,
                                            &stack.back());
                else
                {
                    r = fvm_value_to_string(c, &member, &key);
                    if (r == FVM_OK)
                        r = fvm_value_set_property(c, &owner, key.string, &stack.back());
                }
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - 3);
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
                fvm_value_boolean(&v, !truth(v));
            }
            else if (a.op == 0x0e || a.op == 0x0f || a.op == 0x10 || a.op == 0x11)
            {
                /* Equals, Less, And, Or: deliberately limited primitive semantics. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value rhs = stack.back();
                stack.pop_back();
                FVM_Value &lhs = stack.back();
                if (a.op == 0x0e || a.op == 0x0f)
                {
                    FVM_Value operands[2] = {lhs, rhs}, result;
                    const char *hook = a.op == 0x0e ? "__eq" : "__lt";
                    FVM_Result status =
                        fvm_value_meta_invoke(c, operands, hook, 2, operands, &result);
                    if (status == FVM_NOT_FOUND)
                        status = fvm_value_meta_invoke(c, operands + 1, hook, 2, operands, &result);
                    if (status == FVM_OK)
                    {
                        fvm_value_boolean(&lhs, truth(result));
                        ++pc;
                        continue;
                    }
                    if (status != FVM_NOT_FOUND)
                        return status;
                }
                if (lhs.type != FVM_VALUE_NUMBER || rhs.type != FVM_VALUE_NUMBER)
                    return FVM_UNSUPPORTED;
                int value = 0;
                if (a.op == 0x0e)
                    value = (lhs.number == rhs.number);
                if (a.op == 0x0f)
                    value = (lhs.number < rhs.number);
                if (a.op == 0x10)
                    value = (lhs.number != 0 && rhs.number != 0);
                if (a.op == 0x11)
                    value = (lhs.number != 0 || rhs.number != 0);
                fvm_value_boolean(&lhs, value);
            }
            else if (a.op == 0x17)
            { /* Pop */
                if (stack.empty())
                    return FVM_ERROR;
                stack.pop_back();
            }
            else if (a.op == 0x3c || a.op == 0x41)
            { /* DefineLocal / DeclareLocal */
                if (!locals)
                    return FVM_UNSUPPORTED;
                if (a.op == 0x3c)
                {
                    if (stack.size() < 2 || stack[stack.size() - 2].type != FVM_VALUE_STRING)
                        return FVM_ERROR;
                }
                else if (stack.empty() || stack.back().type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                fvm_detail::string name =
                    stack[a.op == 0x3c ? stack.size() - 2 : stack.size() - 1].string;
                FVM_Value v;
                fvm_value_undefined(&v);
                if (a.op == 0x3c)
                    v = stack.back();
                (*locals)[name] = v;
                stack.resize(stack.size() - (a.op == 0x3c ? 2 : 1));
            }
            else if (a.op == 0x1c)
            {
                if (stack.empty() || stack.back().type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                FVM_Value value;
                FVM_Result r = lookup_variable(c, locals, self, stack.back().string, &value);
                if (r != FVM_OK)
                    return r;
                stack.back() = value;
            }
            else if (a.op == 0x1d)
            { /* SetVariable: name, value */
                if (stack.size() < 2 || stack[stack.size() - 2].type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                fvm_detail::string name = stack[stack.size() - 2].string;
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
                    r = captured ? fvm_object_set(c, captured, name.c_str(), &stack.back())
                        : self && self->resource_kind == 40
                            ? fvm_object_set(c, self, name.c_str(), &stack.back())
                            : fvm_set_global(c, name.c_str(), &stack.back());
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
                FVM_Value b = stack.back();
                stack.pop_back();
                FVM_Value &v = stack.back();
                const char *hook = a.op == 0x0a   ? "__add"
                                   : a.op == 0x0b ? "__sub"
                                   : a.op == 0x0c ? "__mul"
                                                  : "__div";
                FVM_Value operands[2] = {v, b}, result;
                FVM_Result status =
                    fvm_value_meta_invoke(c, &operands[0], hook, 2, operands, &result);
                if (status == FVM_NOT_FOUND)
                    status = fvm_value_meta_invoke(c, &operands[1], hook, 2, operands, &result);
                if (status == FVM_OK)
                {
                    v = result;
                    ++pc;
                    continue;
                }
                if (status != FVM_NOT_FOUND)
                    return status;
                if (v.type != FVM_VALUE_NUMBER || b.type != FVM_VALUE_NUMBER)
                    return FVM_UNSUPPORTED;
                v.metatable = 0;
                if (a.op == 0x0a)
                    v.number += b.number;
                if (a.op == 0x0b)
                    v.number -= b.number;
                if (a.op == 0x0c)
                    v.number *= b.number;
                if (a.op == 0x0d)
                    v.number /= b.number;
            }
            else if (a.op == 0x52 || a.op == 0x53)
            { /* CallMethod: args, argc, receiver, method name */
                if (stack.size() < 3 ||
                    (stack.back().type != FVM_VALUE_STRING && stack.back().type != FVM_VALUE_NULL &&
                     stack.back().type != FVM_VALUE_UNDEFINED) ||
                    stack[stack.size() - 3].type != FVM_VALUE_NUMBER)
                    return FVM_ERROR;
                fvm_detail::string name =
                    stack.back().type == FVM_VALUE_STRING ? stack.back().string : "";
                FVM_Value owner = stack[stack.size() - 2];
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
                double n = stack[stack.size() - 3].number;
                if (!(n >= 0 && n <= 65535) || n != std::floor(n) || stack.size() < (size_t)n + 3)
                    return FVM_ERROR;
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
                FVM_Result lookup =
                    fvm_lookup_native_method(receiver, name.c_str(), &native_method);
                if (lookup != FVM_OK && lookup != FVM_NOT_FOUND && lookup != FVM_UNSUPPORTED)
                    return lookup;
                if (a.op == 0x53)
                {
                    FVM_Value function;
                    r = name.empty() ? FVM_OK
                                     : fvm_object_get(c, receiver, name.c_str(), &function);
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
                else if (lookup == FVM_NOT_FOUND && script != c->script_functions.end())
                    r = call_script(c, script->first.c_str(), script->second, argc,
                                    argc ? &args[0] : 0, receiver, &result);
                else
                    r = fvm_object_call(c, receiver, name.c_str(), argc, argc ? &args[0] : 0,
                                        &result);
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
                fvm_detail::string name = stack.back().string;
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
                r = lookup_variable(c, locals, self, name.c_str(), &callable, &defined,
                                    &call_receiver);
                if (r != FVM_OK)
                    return r;
                if (callable.type == FVM_VALUE_OBJECT && callable.object->invocable.callback)
                    r = fvm_function_invoke(c, callable.object, call_receiver, argc, args.begin(),
                                            &result);
                else if (defined)
                    r = FVM_UNSUPPORTED;
                else if (fn != c->script_functions.end())
                    r = call_script(c, fn->first.c_str(), fn->second, argc, args.begin(), c->global,
                                    &result);
                else
                    r = fvm_call(c, name.c_str(), argc, args.begin(), &result);
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
                    FVM_Value v = stack.back();
                    stack.pop_back();
                    take = truth(v);
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
extern "C" FVM_Result fvm_avm1_execute(FVM_Context *c, const void *data, size_t size)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result r = fvm_avm1_inspect(data, size, &info);
    if (r != FVM_OK)
        return r;
    return execute(c, (const unsigned char *)data, size, 0, 0, c->global, 0);
}
extern "C" FVM_Result fvm_swf_execute_actions(FVM_Context *c)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    if (!c->swf_loaded)
        return FVM_ERROR;
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

extern "C" FVM_Result fvm_avm1_call(FVM_Context *c, const char *name, FVM_Object *self,
                                    unsigned int argc, const FVM_Value *args, FVM_Value *out)
{
    if (!c || !name || !out || (self && self->owner != c) || (argc && !args))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < argc; ++i)
        if (fvm_validate_value(c, args + i) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    FVM_Value function;
    FVM_Result lookup = fvm_get_global(c, name, &function);
    if (lookup != FVM_OK)
        return lookup;
    if (function.type == FVM_VALUE_OBJECT && function.object->invocable.callback)
        return fvm_function_invoke(c, function.object, self, argc, args, out);
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::iterator it =
        c->script_functions.find(name);
    if (it == c->script_functions.end())
        return FVM_NOT_FOUND;
    return call_script(c, name, it->second, argc, args, self ? self : c->global, out);
}

FVM_Result fvm_avm1_invoke_script(FVM_Context *c, const FVM_ScriptFunction &body,
                                  FVM_Object *closure, FVM_Object *receiver, unsigned int argc,
                                  const FVM_Value *args, FVM_Value *out, FVM_Object *callee)
{
    return call_script(c, body.name.empty() ? "<function>" : body.name.c_str(), body, argc, args,
                       receiver ? receiver : c->global, out, closure, callee);
}

extern "C" FVM_Result fvm_avm1_execute_receiver(FVM_Context *c, const void *data, size_t size,
                                                FVM_Object *receiver)
{
    if (!c || !receiver || receiver->owner != c)
        return FVM_INVALID_ARGUMENT;
    FVM_AVM1Info info;
    info.struct_size = sizeof(info);
    FVM_Result r = fvm_avm1_inspect(data, size, &info);
    if (r != FVM_OK)
        return r;
    return execute(c, (const unsigned char *)data, size, 0, 0, receiver, 0);
}
