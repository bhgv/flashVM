#include "../core/runtime.hpp"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
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
        return value.string && value.string[0];
    case FVM_VALUE_OBJECT:
        return true;
    default:
        return false;
    }
}

static FVM_Result own_string(FVM_Context *context, const char *text, FVM_Value *value)
{
    size_t length = std::strlen(text);
    char *copy = new (std::nothrow) char[length + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;
    std::memcpy(copy, text, length + 1);
    try
    {
        context->strings.push_back(copy);
    }
    catch (...)
    {
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    return fvm_value_string(value, copy);
}
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited);
/* A call frame owns parameter bindings and its receiver; script depth is restored
 * even when the interpreter reports an error. */
static FVM_Result call_script(FVM_Context *c, const FVM_ScriptFunction &fn, unsigned int argc,
                              const FVM_Value *args, FVM_Object *receiver, FVM_Value *result)
{
    if (c->script_depth >= 64)
        return FVM_ERROR;
    fvm_detail::map<fvm_detail::string, FVM_Value> frame;
    try
    {
        for (size_t i = 0; i < fn.parameters.size(); ++i)
        {
            FVM_Value value;
            fvm_value_undefined(&value);
            if (i < argc)
                value = args[i];
            frame[fn.parameters[i]] = value;
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
    ++c->script_depth;
    FVM_Result status = execute(c, &snapshot.body[0], snapshot.body.size(), result, &frame,
                                receiver, &snapshot.constants);
    --c->script_depth;
    /* A returned bytecode literal must outlive the function snapshot. */
    if (status == FVM_OK && result && result->type == FVM_VALUE_STRING)
    {
        size_t len = std::strlen(result->string);
        char *copy = new (std::nothrow) char[len + 1];
        if (!copy)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(copy, result->string, len + 1);
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
                          const fvm_detail::vector<fvm_detail::string> *inherited)
{
    fvm_detail::vector<fvm_detail::string> constants;
    fvm_detail::vector<FVM_Value> registers;
    fvm_detail::vector<Action> actions;
    fvm_detail::vector<FVM_Value> stack;
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
    unsigned long steps = 0;
    if (!c || (!data && size))
        return FVM_INVALID_ARGUMENT;
    if (returned)
        fvm_value_undefined(returned);
    try
    {
        if (inherited)
            constants = *inherited;
        registers.resize(4);
        for (size_t reg = 0; reg < registers.size(); ++reg)
            fvm_value_undefined(&registers[reg]);
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
            }
            a.next = pos;
            actions.push_back(a);
            if (!a.op)
                break;
        }
        if (actions.empty() || actions.back().op != 0 || pos != size)
            return FVM_INVALID_ARGUMENT;
        while (pc < actions.size())
        {
            const Action &a = actions[pc];
            ++steps;
            if (c->runtime->config.instruction_limit &&
                steps > c->runtime->config.instruction_limit)
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
                return FVM_OK;
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
            { /* DefineFunction: named, zero parameters */
                size_t p = a.payload, end = p + a.length, start = p;
                while (p < end && data[p])
                    ++p;
                if (p == end || p == start)
                    return FVM_INVALID_ARGUMENT;
                fvm_detail::string name((const char *)(data + start), p - start);
                ++p;
                if (end - p < 4)
                    return FVM_INVALID_ARGUMENT;
                unsigned int count = u16(data + p);
                p += 2;
                FVM_ScriptFunction fn;
                fn.constants = constants;
                unsigned int param;
                for (param = 0; param < count; ++param)
                {
                    size_t begin = p;
                    while (p < end && data[p])
                        ++p;
                    if (p == end)
                        return FVM_INVALID_ARGUMENT;
                    fn.parameters.push_back(
                        fvm_detail::string((const char *)(data + begin), p - begin));
                    ++p;
                }
                if (end - p < 2)
                    return FVM_INVALID_ARGUMENT;
                unsigned int body_size = u16(data + p);
                p += 2;
                if (body_size != end - p)
                    return FVM_INVALID_ARGUMENT;
                fn.body.assign(data + p, data + end);
                /* A function body can omit End; append one for the decoder. */
                if (fn.body.empty() || fn.body.back() != 0)
                    fn.body.push_back(0);
                c->script_functions[name] = fn;
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
                FVM_Result result =
                    fvm_construct(c, stack.back().string, argc, args.begin(), &object);
                if (result != FVM_OK)
                    return result;
                stack.resize(stack.size() - argc - 2);
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = object;
                stack.push_back(value);
            }
            else if (a.op == 0x4e)
            { /* GetMember */
                if (stack.size() < 2 || stack.back().type != FVM_VALUE_STRING ||
                    stack[stack.size() - 2].type != FVM_VALUE_OBJECT)
                    return FVM_ERROR;
                FVM_Value v;
                FVM_Result r =
                    fvm_object_get(c, stack[stack.size() - 2].object, stack.back().string, &v);
                if (r != FVM_OK)
                    return r;
                stack.pop_back();
                stack.back() = v;
            }
            else if (a.op == 0x4f)
            { /* SetMember */
                if (stack.size() < 3 || stack[stack.size() - 2].type != FVM_VALUE_STRING ||
                    stack[stack.size() - 3].type != FVM_VALUE_OBJECT)
                    return FVM_ERROR;
                FVM_Result r = fvm_object_set(c, stack[stack.size() - 3].object,
                                              stack[stack.size() - 2].string, &stack.back());
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
            { /* GetVariable */
                if (stack.empty() || stack.back().type != FVM_VALUE_STRING)
                    return FVM_ERROR;
                FVM_Value v;
                FVM_Result r = FVM_OK;
                fvm_detail::map<fvm_detail::string, FVM_Value>::iterator local =
                    locals ? locals->find(stack.back().string)
                           : fvm_detail::map<fvm_detail::string, FVM_Value>::iterator();
                if (locals && local != locals->end())
                    v = local->second;
                else if (self && std::strcmp(stack.back().string, "this") == 0)
                {
                    fvm_value_undefined(&v);
                    v.type = FVM_VALUE_OBJECT;
                    v.object = self;
                }
                else
                    r = fvm_get_global(c, stack.back().string, &v);
                if (r != FVM_OK)
                    return r;
                stack.back() = v;
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
                    r = fvm_set_global(c, name.c_str(), &stack.back());
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
                if (v.type != FVM_VALUE_NUMBER || b.type != FVM_VALUE_NUMBER)
                    return FVM_UNSUPPORTED;
                if (a.op == 0x0a)
                    v.number += b.number;
                if (a.op == 0x0b)
                    v.number -= b.number;
                if (a.op == 0x0c)
                    v.number *= b.number;
                if (a.op == 0x0d)
                    v.number /= b.number;
            }
            else if (a.op == 0x52)
            { /* CallMethod: args, argc, receiver, method name */
                if (stack.size() < 3 || stack.back().type != FVM_VALUE_STRING ||
                    stack[stack.size() - 2].type != FVM_VALUE_OBJECT ||
                    stack[stack.size() - 3].type != FVM_VALUE_NUMBER)
                    return FVM_ERROR;
                fvm_detail::string name = stack.back().string;
                FVM_Object *receiver = stack[stack.size() - 2].object;
                double n = stack[stack.size() - 3].number;
                if (n < 0 || n > 65535 || n != (unsigned int)n || stack.size() < (size_t)n + 3)
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
                if (lookup != FVM_OK && lookup != FVM_NOT_FOUND)
                    return lookup;
                if (lookup == FVM_NOT_FOUND && script != c->script_functions.end())
                    r = call_script(c, script->second, argc, argc ? &args[0] : 0, receiver,
                                    &result);
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
                if (n < 0 || n > 65535 || n != (unsigned int)n || stack.size() < (size_t)n + 2)
                    return FVM_ERROR;
                unsigned int argc = (unsigned int)n;
                fvm_detail::vector<FVM_Value> args(argc);
                for (i = 0; i < argc; ++i)
                    args[i] = stack[stack.size() - 3 - i];
                FVM_Value result;
                FVM_Result r;
                fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::const_iterator fn =
                    c->script_functions.find(name);
                if (fn != c->script_functions.end())
                {
                    r = call_script(c, fn->second, argc, argc ? &args[0] : 0, c->global, &result);
                }
                else
                    r = fvm_call(c, name.c_str(), argc, argc ? &args[0] : 0, &result);
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
    return FVM_INVALID_ARGUMENT;
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
        if (args[i].type < FVM_VALUE_UNDEFINED || args[i].type > FVM_VALUE_OBJECT ||
            (args[i].type == FVM_VALUE_OBJECT && (!args[i].object || args[i].object->owner != c)) ||
            (args[i].type == FVM_VALUE_STRING && !args[i].string))
            return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string, FVM_ScriptFunction>::iterator it =
        c->script_functions.find(name);
    if (it == c->script_functions.end())
        return FVM_NOT_FOUND;
    return call_script(c, it->second, argc, args, self ? self : c->global, out);
}
