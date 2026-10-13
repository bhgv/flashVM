/* FlashVM: AVM1 bytecode decoding and execution: interpreter.
 * File: src/avm1/interpreter.cpp.
 */

#include "function_record.hpp"
#include "coercion.hpp"
#include "../standard/function_state.hpp"
#include "../standard/inheritance.hpp"
#include "../standard/super.hpp"
#include "../standard/base_state.hpp"
#include "../standard/primitive_state.hpp"
#include "../standard/movieclip_state.hpp"
#include "flashvm/function.h"
#include "../standard/string_order.hpp"
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
    if (value.type == FVM_VALUE_STRING && context->swf_loaded &&
        context->swf_info.version < 7)
    {
        double number = 0;
        FVM_Result result = fvm_avm1_number(context, &value, &number);
        if (result != FVM_OK)
            return result;
        out = number != 0 && number == number;
    }
    else
        out = truth(value);
    return FVM_OK;
}

/* Legacy ordinary coercion uses an empty undefined string; Trace keeps its own spelling. */
static FVM_Result action_string(FVM_Context *context, const FVM_Value &value, FVM_Value *out)
{
    if (value.type == FVM_VALUE_UNDEFINED && context->swf_loaded &&
        context->swf_info.version < 7)
        return fvm_value_string(out, "");
    if (value.type == FVM_VALUE_BOOLEAN && context->swf_loaded &&
        context->swf_info.version < 5)
        return fvm_value_string(out, value.boolean ? "1" : "0");
    if (value.type == FVM_VALUE_OBJECT && context->swf_loaded && value.object->resource_kind == 40)
        return fvm_movieclip_reference_string(context, value.object, out);
    if (value.type == FVM_VALUE_OBJECT && context->swf_loaded && value.object->resource_kind != 40)
    {
        if (value.object->resource_kind == 26 && value.object->resource_result_valid &&
            value.object->resource_result.type == FVM_VALUE_STRING)
        {
            *out = value.object->resource_result;
            return FVM_OK;
        }
        if (context->string_depth >= 64)
            return FVM_ERROR;
        /* [GC:] The caller retains the owner; invocation roots its selected method. */
        struct StringScope
        {
            FVM_Context *context; /* Owner of the string-coercion recursion counter. */
            /* Bound string callback reentry before resolving toString. */
            explicit StringScope(FVM_Context *owner) : context(owner)
            {
                ++context->string_depth;
            }
            /* Restore recursion depth after invocation or allocation failure. */
            ~StringScope()
            {
                --context->string_depth;
            }
        } coercion(context);
        FVM_Value method, result;
        fvm_value_undefined(&result);
        FVM_Result status = fvm_object_get_bytes(context, value.object, "toString", 8, &method);
        if (status != FVM_OK)
            return status;
        if (method.type == FVM_VALUE_OBJECT &&
            (method.object->invocable.callback || method.object->callable.callback))
            status = fvm_function_invoke(context, method.object, value.object, 0, 0, &result);
        if (status != FVM_OK)
            return status;
        if (result.type == FVM_VALUE_STRING)
        {
            *out = result;
            return FVM_OK;
        }
        return fvm_value_string(out, value.object->invocable.callback || value.object->callable.callback
                                        ? "[type Function]" : "[type Object]");
    }
    return fvm_value_to_string(context, &value, out);
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

/* Compute and return date hint. */
static bool date_hint(const FVM_Value &value)
{
    return value.type == FVM_VALUE_OBJECT && value.object->resource_kind == 34;
}

/* Compute and return strict equal. */
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

/* [GC:] Retain both loose-equality values while recursive/scripted conversion replaces them. */
struct EqualityRoots
{
    FVM_Context *context; /* Context owning this temporary root frame. */
    FVM_GCRootFrame frame; /* [GC:] Frame linked above the caller's operand roots. */
    fvm_detail::vector<FVM_Value> values; /* [GC:] Mutable operands including converted strings. */
  private:
    /* [GC:] Copying a linked frame would break the collector's LIFO chain. */
    EqualityRoots(const EqualityRoots &);
    /* [GC:] Root-frame assignment is unavailable. */
    EqualityRoots &operator=(const EqualityRoots &);
  public:
    /* [GC:] Publish initialized values only after the vector allocation succeeds. */
    EqualityRoots(FVM_Context *owner, const FVM_Value &left, const FVM_Value &right)
        : context(owner), values(2)
    {
        values[0] = left;
        values[1] = right;
        frame.previous = context->gc_frames;
        frame.stack = &values;
        frame.locals = 0;
        frame.receiver = 0;
        context->gc_frames = &frame;
    }
    /* [GC:] Restore the caller frame on every return and exception path. */
    ~EqualityRoots()
    {
        context->gc_frames = frame.previous;
    }
};

/* Compute and return equal. */
static FVM_Result equal(FVM_Context *c, FVM_Value input_a, FVM_Value input_b, bool &result,
                        unsigned int depth = 0)
{
    if (depth > 8)
        return FVM_ERROR;
    EqualityRoots roots(c, input_a, input_b);
    FVM_Value &a = roots.values[0], &b = roots.values[1];
    if (c->swf_loaded && c->swf_info.version <= 5)
    {
        FVM_Result status = fvm_avm1_primitive(c, &b, false, &b);
        if (status == FVM_OK)
            status = fvm_avm1_primitive(c, &a, false, &a);
        if (status != FVM_OK)
            return status;
    }
    if (a.type == b.type)
    {
        /* Loaded AVM1 targets the Flash Player 7/8 NaN rule; StrictEquals remains distinct. */
        result = strict_equal(a, b) ||
                 (c->swf_loaded && a.type == FVM_VALUE_NUMBER && a.number != a.number &&
                  b.number != b.number);
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
        FVM_Result r = fvm_avm1_number(c, &a, &x);
        if (r == FVM_OK)
            r = fvm_avm1_number(c, &b, &y);
        result = x == y;
        return r;
    }
    if (a.type == FVM_VALUE_OBJECT && b.type != FVM_VALUE_OBJECT)
    {
        FVM_Result r = c->swf_loaded ? fvm_avm1_primitive(c, &a, false, &a)
                                     : fvm_to_primitive(c, &a, date_hint(a), &a);
        if (r == FVM_OK && a.type == FVM_VALUE_OBJECT)
        {
            result = false;
            return FVM_OK;
        }
        return r == FVM_OK ? equal(c, a, b, result, depth + 1) : r;
    }
    if (b.type == FVM_VALUE_OBJECT && a.type != FVM_VALUE_OBJECT)
    {
        FVM_Result r = c->swf_loaded ? fvm_avm1_primitive(c, &b, false, &b)
                                     : fvm_to_primitive(c, &b, date_hint(b), &b);
        if (r == FVM_OK && b.type == FVM_VALUE_OBJECT)
        {
            result = false;
            return FVM_OK;
        }
        return r == FVM_OK ? equal(c, a, b, result, depth + 1) : r;
    }
    result = false;
    return FVM_OK;
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
    if (self && name == fvm_detail::string("this"))
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
            return fvm_object_get_bytes(c, self, name.data(), name.size(), out);
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

/* Declare a local cell or top-level clip/global binding without erasing an existing var declaration. */
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
    FVM_Object *destination = self && self->resource_kind == 40 ? self : context->global;
    if (!initialized)
    {
        FVM_Object *cursor = destination;
        unsigned int depth = 0;
        while (cursor && depth++ < 1024)
        {
            int own = 0;
            FVM_Result status = fvm_object_has_own_bytes(context, cursor, name.data(), name.size(), &own);
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

/* Define Execution struct and its representation. */
struct Execution
{
    fvm_detail::vector<fvm_detail::string> constants;
    fvm_detail::vector<FVM_Value> registers, stack;
    unsigned long steps;
    bool returned;
    /* Initialize Execution and its owned state. */
    Execution() : steps(0), returned(false)
    {
    }
};
/* Execute the current value. */
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared = 0, size_t base = 0, unsigned int depth = 0);
/* A call frame owns parameter bindings and its receiver; script depth is restored
 * even when the interpreter reports an error. */
/* Call script. */
static FVM_Result call_script(FVM_Context *c, const fvm_detail::string &name, const FVM_ScriptFunction &fn,
                              unsigned int argc, const FVM_Value *args, FVM_Object *receiver,
                              FVM_Value *result, FVM_Object *closure = 0, FVM_Object *callee = 0)
{
    if (c->script_depth >= 64)
        return FVM_ERROR;

    /* Initialization / Инициализация локального состояния. */

    Execution execution;
    try
    {
        execution.constants = fn.constants;
        execution.registers.resize(fn.function2 ? fn.register_count + 1 : 4);
        for (size_t i = 0; i < execution.registers.size(); ++i)
            fvm_value_undefined(&execution.registers[i]);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_Object *activation = 0;
    FVM_Result activation_result = fvm_object_create(c, &activation);
    if (activation_result != FVM_OK)
        return activation_result;
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
        ConstructionRoots(FVM_Context *context, FVM_Object *activation,
                          fvm_detail::vector<FVM_Value> *roots, fvm_detail::vector<FVM_Value> *regs)
            : c(context)
        {
            /* [GC:] Save the caller root chain before attaching this activation. */
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
        {
            /* [GC:] Remove both temporary frames on scope exit. */
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
        FVM_Value implicit_super;
        fvm_value_undefined(&implicit_super);
        if (!fn.function2 || !(fn.flags & 32) || (fn.flags & 16))
        {
            FVM_Result made = fvm_super_create(c, callee, receiver, &implicit_super);
            if (made != FVM_OK)
                return made;
            /* [GC:] Retain the facade while the rest of the activation is constructed. */
            constructing.push_back(implicit_super);
        }
        FVM_Object *arguments = 0;
        FVM_Result stored = fvm_object_create(c, &arguments);
        if (stored != FVM_OK)
            return stored;
        arguments->resource_kind = 30;
        capture.object = arguments;
        /* [GC:] Retain this value while subsequent call setup may execute script code. */
        constructing.push_back(capture);
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

        FVM_Value implicit_arguments = value;
        if (fn.function2 && (fn.flags & 8))
            fvm_value_undefined(&value);
        stored = fvm_object_set(c, activation, "arguments", &value);
        if (stored != FVM_OK)
            return stored;
        /* Explicit parameters can shadow the implicit arguments binding. */

        {
            FVM_Value named_this;
            fvm_value_undefined(&named_this);
            if (!fn.function2 || !(fn.flags & 2))
            {
                named_this.type = FVM_VALUE_OBJECT;
                named_this.object = receiver ? receiver : c->global;
            }
            stored = fvm_object_set(c, activation, "this", &named_this);
            if (stored != FVM_OK)
                return stored;
            FVM_Value named_super = implicit_super;
            if (fn.function2 && (fn.flags & 32))
                fvm_value_undefined(&named_super);
            stored = fvm_object_set(c, activation, "super", &named_super);
            if (stored != FVM_OK)
                return stored;
        }
        for (size_t i = 0; i < fn.parameters.size(); ++i)
        {
            FVM_Value value;
            fvm_value_undefined(&value);
            if (i < argc)
                value = args[i];

            if (fn.function2 && fn.parameter_registers[i])
            {
                execution.registers[fn.parameter_registers[i]] = value;
                continue;
            }
            FVM_Result stored = fvm_object_set_bytes(c, activation, fn.parameters[i].data(), fn.parameters[i].size(), &value);
            if (stored != FVM_OK)
                return stored;
        }
        if (fn.function2)
        {
            FVM_Value common[6];
            for (unsigned int i = 0; i < 6; ++i)
                fvm_value_undefined(common + i);
            common[0].type = FVM_VALUE_OBJECT;
            common[0].object = receiver ? receiver : c->global;
            common[1] = implicit_arguments;
            common[2] = implicit_super;
            if (receiver && (fn.flags & 64))
            {
                stored = fvm_object_get(c, receiver, "_root", common + 3);
                if (stored != FVM_OK)
                    return stored;
                /* [GC:] Retain this value while subsequent call setup may execute script code. */
                constructing.push_back(common[3]);
            }
            if ((fn.flags & 64) && common[3].type == FVM_VALUE_UNDEFINED)
            {
                stored = fvm_get_global(c, "_root", common + 3);
                if (stored != FVM_OK)
                    return stored;
            }
            if (receiver && (fn.flags & 128))
            {
                stored = fvm_object_get(c, receiver, "_parent", common + 4);
                if (stored != FVM_OK)
                    return stored;
            }
            common[5].type = FVM_VALUE_OBJECT;
            common[5].object = c->global;
            const unsigned int bits[] = {1, 4, 16, 64, 128, 256};
            unsigned int slot = 1;
            for (unsigned int i = 0; i < 6; ++i)
                if (fn.flags & bits[i])
                    execution.registers[slot++] = common[i];
        }
        fvm_detail::map<fvm_detail::string, bool> seen;
        for (size_t i = fn.parameters.size(); i; --i)
        {
            size_t index = i - 1;
            if (fn.function2 && fn.parameter_registers[index])
                continue;
            fvm_detail::string name;
            stored = fvm_property_key(fn.parameters[index].data(), fn.parameters[index].size(), name);
            if (stored != FVM_OK)
                return stored;
            if (seen.find(name) != seen.end())
                continue;
            seen[name] = true;
            if (index >= argc)
                continue;
            stored = fvm_function_map_argument(c, arguments, (unsigned long)index, activation, name);
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
    try
    {
        DebugFrame debug(c);
        c->debug_function = name;
        c->debug_line = 0;
        /* [GC:] Restore script call depth after both successful execution and preparation OOM. */
        struct DepthScope
        {
            FVM_Context *context;
            unsigned int previous;
            /* [GC:] Enter one script activation after preserving the caller's exact depth. */
            explicit DepthScope(FVM_Context *c) : context(c), previous(c->script_depth)
            {
                ++context->script_depth;
            }
            /* [GC:] Restore depth even if lexical-scope registration throws before bytecode entry.
             */
            ~DepthScope()
            {
                context->script_depth = previous;
            }
        } depth(c);
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
        status = execute(c, &snapshot.body[0], snapshot.body.size(), result, &frame, receiver,
                         &snapshot.constants, &execution);
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
/* Execute the current value. */
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size,
                          FVM_Value *returned,
                          fvm_detail::map<fvm_detail::string, FVM_Value> *locals, FVM_Object *self,
                          const fvm_detail::vector<fvm_detail::string> *inherited,
                          Execution *shared, size_t base, unsigned int depth)
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
        /* Initialize Root Guard and its owned state. */
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        RootGuard(FVM_Context *c, fvm_detail::vector<FVM_Value> *s,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  fvm_detail::map<fvm_detail::string, FVM_Value> *l, FVM_Object *r,
                  /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                  fvm_detail::vector<FVM_Value> *reg)
            /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
            : context(c)
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
            register_frame.receiver = 0;
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
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
        }
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
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
        if ((!shared && (actions.empty() || actions.back().op != 0)) || pos != size)
            return FVM_INVALID_ARGUMENT;
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
                FVM_Result named = fvm_property_key((const char *)data + record.name, record.name_size,
                                                     fn.name);
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
                            r = fvm_object_set_bytes(c, object, text.string, fvm_string_size(text), &value);
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
            /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
            {
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (!self || self->resource_kind != 40)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return FVM_UNSUPPORTED;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                const char *method =
                    a.op == 0x04 ? "nextFrame"
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    : a.op == 0x05 ? "prevFrame"
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    : a.op == 0x06 ? "play"
                                   /* [GC:] Операция в блоке управления достижимостью и временем
                                      жизни объектов. */
                                   : "stop";
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Value frame, result;
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                unsigned int count = 0;
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (a.op == 0x81 || a.op == 0x8c || a.op == 0x9f)
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                {
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    count = 1;
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    bool playing = false;
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    FVM_Object *state = self->resource_result.object;
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (!self->resource_result_valid ||
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        self->resource_result.type != FVM_VALUE_OBJECT || !state)
                        /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                        return FVM_INVALID_ARGUMENT;
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator play =
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                           объектов. */
                        state->properties.find("playing");
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (play != state->properties.end())
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        playing = play->second.number != 0;
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    if (a.op == 0x81)
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                    {
                        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                        if (a.length != 2)
                            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                            return FVM_INVALID_ARGUMENT;
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        fvm_value_number(&frame, u16(data + a.payload) + 1);
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                    }
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    else if (a.op == 0x8c)
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                    {
                        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                        if (!a.length || !std::memchr(data + a.payload, 0, a.length))
                            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                            return FVM_INVALID_ARGUMENT;
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        FVM_Result r = own_string(c, (const char *)data + a.payload, &frame);
                        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                        if (r != FVM_OK)
                            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                            return r;
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                    }
                    /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                    else
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                    {
                        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                        if ((a.length != 1 && a.length != 3) || stack.empty() ||
                            /* [GC:] Операция в блоке управления достижимостью и временем жизни
                               объектов. */
                            /* [GC:] Операция в блоке управления достижимостью и временем жизни
                               объектов. */
                            (data[a.payload] & ~3) ||
                            /* [GC:] Операция в блоке управления достижимостью и временем жизни
                               объектов. */
                            a.length != ((data[a.payload] & 2) ? 3U : 1U))
                            /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                            return FVM_INVALID_ARGUMENT;
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        frame = stack.back();
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        stack.pop_back();
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        /* [GC:] Операция в блоке управления достижимостью и временем жизни
                         * объектов. */
                        playing = (data[a.payload] & 1) != 0;
                        /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                        if ((data[a.payload] & 2) && frame.type == FVM_VALUE_NUMBER)
                            /* [GC:] Операция в блоке управления достижимостью и временем жизни
                             * объектов. */
                            /* [GC:] Операция в блоке управления достижимостью и временем жизни
                             * объектов. */
                            frame.number += u16(data + a.payload + 1);
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                        /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней.
                         */
                    }
                    /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                    method = playing ? "gotoAndPlay" : "gotoAndStop";
                    /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
                }
                /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
                FVM_Result r = fvm_object_call(c, self, method, count, count ? &frame : 0, &result);
                /* [GC:] Выбираем безопасную ветку обработки состояния сборки или корней. */
                if (r != FVM_OK)
                    /* [GC:] Возвращаем результат операции сборщика или защиты корней. */
                    return r;
                /* [GC:] Граница блока, управляющего жизненным циклом объектов или корней. */
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
                    status = action_string(c, stack.back(), &request[1]);
                    if (status == FVM_OK)
                        status = action_string(c, stack[stack.size() - 2], &request[0]);
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
                        status = own_string(c, (const char *)separator + 1, &request[1],
                                            remaining - 1);
                }
                if (status != FVM_OK)
                    return status;
                fvm_value_number(&request[2], flags);
                /* Copy the sink before delivery so replacement applies to the next request. */
                FVM_NativeEntry sink = c->avm1_url;
                if (!sink.callback)
                    return FVM_UNSUPPORTED;
                FVM_Value ignored;
                fvm_value_undefined(&ignored);
                {
                    /* [GC:] Host URL callbacks share the ordinary native collection barrier. */
                    FVM_NativeCallGuard guard(c);
                    status = sink.callback(c, c->global, 3, request.begin(), &ignored, sink.userdata);
                }
                if (status != FVM_OK)
                    return status;
                if (a.op == 0x9a)
                    stack.resize(stack.size() - 2);
            }
            else if (a.op == 0x9e)
            { /* Call executes frame actions without seeking or placing frame controls. */
                if (a.length || stack.empty())
                    return a.length ? FVM_INVALID_ARGUMENT : FVM_ERROR;
                FVM_Value argument = stack.back();
                FVM_Result status = FVM_OK;
                if (argument.type != FVM_VALUE_NUMBER)
                {
                    FVM_Value converted;
                    status = action_string(c, argument, &converted);
                    if (status == FVM_OK)
                        argument = converted;
                }
                if (status != FVM_OK)
                    return status;
                /* [GC:] Keep the converted label live until the nested frame has finished. */
                stack.back() = argument;
                FVM_Object *target = self && self->resource_kind == 40 ? self : 0;
                if (!target)
                {
                    FVM_Value root;
                    status = fvm_get_global(c, "_root", &root);
                    if (status != FVM_OK)
                        return status;
                    if (root.type == FVM_VALUE_OBJECT && root.object->resource_kind == 40)
                        target = root.object;
                }
                if (!target)
                    return FVM_UNSUPPORTED;
                status = fvm_movieclip_call_frame(c, target, argument);
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
                    status = action_string(c, stack[stack.size() - 2], &left);
                    if (status == FVM_OK)
                        status = action_string(c, stack.back(), &right);
                }
                else
                {
                    status = action_string(c, stack.back(), &right);
                    if (status == FVM_OK)
                        status = action_string(c, stack[stack.size() - 2], &left);
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
                    fvm_value_boolean(&output, a.op == 0x13 ? order == 0
                                                  : a.op == 0x29 ? order < 0 : order > 0);
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
                FVM_Result status = action_string(c, stack.back(), &text);
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
                /* [GC:] Retain all three operands while conversion callbacks may invoke script. */
                double length_number = 0, start_number = 0;
                FVM_Result status = fvm_avm1_number(c, &stack.back(), &length_number);
                if (status == FVM_OK)
                    status = fvm_avm1_number(c, &stack[stack.size() - 2], &start_number);
                FVM_Value text, output;
                if (status == FVM_OK)
                    status = action_string(c, stack[stack.size() - 3], &text);
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
                double signed_start = signed_bits(integer_bits(start_number));
                double signed_length = signed_bits(integer_bits(length_number));
                size_t start = signed_start >= 1 ? (size_t)signed_start - 1 : 0;
                size_t end = units.size();
                if (signed_length >= 0 && start <= end && (size_t)signed_length <= end - start)
                    end = start + (size_t)signed_length;
                if (start > end)
                    start = end;
                fvm_detail::vector<unsigned short> selected(end - start);
                for (size_t unit = start; unit < end; ++unit)
                    selected[unit - start] = (unsigned short)units[unit];
                status = fvm_value_string_utf16(c, selected.begin(), selected.size(), &output);
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
                FVM_Result status = action_string(c, stack.back(), &text);
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
            { /* AsciiToChar/MBAsciiToChar wrap to Uint16; zero is empty and surrogates are replaced. */
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
                    r = action_string(c, input, &output);
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
                if (a.op == 0x66)
                {
                    bool matches = strict_equal(stack[stack.size() - 2], stack.back());
                    stack.pop_back();
                    fvm_value_boolean(&stack.back(), matches);
                    ++pc;
                    continue;
                }
                /* [GC:] Retain transformed primitives and derived strings across subsequent callbacks. */
                fvm_detail::vector<FVM_Value> converted(4);
                converted[0] = stack[stack.size() - 2];
                converted[1] = stack.back();
                fvm_value_undefined(&converted[2]);
                fvm_value_undefined(&converted[3]);
                RootGuard converted_roots(c, &converted, 0, 0, &registers);
                FVM_Value &left = converted[0], &right = converted[1];
                FVM_Value output;
                FVM_Result r = FVM_OK;
                if (a.op != 0x66)
                {
                    const char *hook = a.op == 0x47   ? "__add"
                                       : a.op == 0x48 ? "__lt"
                                       : a.op == 0x67 ? "__gt"
                                                      : "__eq";
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
                if (a.op == 0x49)
                {
                    bool matches = false;
                    r = equal(c, left, right, matches);
                    fvm_value_boolean(&output, matches);
                }
                else
                {
                    if (a.op == 0x47)
                    {
                        r = fvm_avm1_primitive(c, &right, true, &right);
                        if (r == FVM_OK)
                            r = fvm_avm1_primitive(c, &left, true, &left);
                    }
                    else
                    {
                        FVM_Value &first = a.op == 0x67 ? right : left;
                        FVM_Value &second = a.op == 0x67 ? left : right;
                        r = fvm_avm1_primitive(c, &first, false, &first);
                        if (r == FVM_OK && c->swf_loaded && first.type == FVM_VALUE_OBJECT &&
                            first.object->resource_kind != 40)
                        {
                            stack.pop_back();
                            fvm_value_boolean(&stack.back(), 0);
                            ++pc;
                            continue;
                        }
                        if (r == FVM_OK)
                            r = fvm_avm1_primitive(c, &second, false, &second);
                        if (r == FVM_OK && c->swf_loaded && second.type == FVM_VALUE_OBJECT &&
                            second.object->resource_kind != 40)
                        {
                            stack.pop_back();
                            fvm_value_boolean(&stack.back(), 0);
                            ++pc;
                            continue;
                        }
                    }
                    if (r == FVM_OK && a.op == 0x47 &&
                        (left.type == FVM_VALUE_STRING || right.type == FVM_VALUE_STRING))
                    {
                        FVM_Value &x = converted[2], &y = converted[3];
                        r = action_string(c, left, &x);
                        if (r == FVM_OK)
                            r = action_string(c, right, &y);
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
                    else if (r == FVM_OK && (a.op == 0x48 || a.op == 0x67) &&
                             left.type == FVM_VALUE_STRING && right.type == FVM_VALUE_STRING)
                    {
                        int order = 0;
                        r = fvm_string_order(left, right, order);
                        fvm_value_boolean(&output, a.op == 0x67 ? order > 0 : order < 0);
                    }
                    else if (r == FVM_OK)
                    {
                        double x = 0, y = 0;
                        r = fvm_avm1_number(c, &left, &x);
                        if (r == FVM_OK)
                            r = fvm_avm1_number(c, &right, &y);
                        if (a.op == 0x47)
                            fvm_value_number(&output, x + y);
                        else if (x != x || y != y)
                            fvm_value_undefined(&output);
                        else
                            fvm_value_boolean(&output, a.op == 0x67 ? x > y : x < y);
                    }
                }
                if (r != FVM_OK)
                    return r;
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
                /* Coerce the original left operand first; user callbacks observe this order. */
                double left = 0, right = 0;
                /* Remainder and bit operations convert the top/right operand first. */
                FVM_Result r = fvm_avm1_number(c, &stack.back(), &right);
                if (r == FVM_OK)
                    r = fvm_avm1_number(c, &stack[stack.size() - 2], &left);
                if (r != FVM_OK)
                    return r;
                double result = 0;
                if (a.op == 0x3f)
                {
                    if (!right || left != left || right != right ||
                        std::fabs(left) == std::numeric_limits<double>::infinity())
                        result = std::numeric_limits<double>::quiet_NaN();
                    else if (std::fabs(right) == std::numeric_limits<double>::infinity())
                        result = left;
                    else
                        result = std::fmod(left, right);
                }
                else
                {
                    unsigned long value = integer_bits(left), operand = integer_bits(right);
                    unsigned int shift = (unsigned int)(operand & 31);
                    unsigned long bits = 0;
                    if (a.op == 0x60)
                        bits = value & operand;
                    else if (a.op == 0x61)
                        bits = value | operand;
                    else if (a.op == 0x62)
                        bits = value ^ operand;
                    else if (a.op == 0x63)
                        bits = (value << shift) & 0xffffffffUL;
                    else
                    {
                        bits = value >> shift;
                        /* Arithmetic right shift is implemented with unsigned masks, avoiding
                         * implementation-defined signed shifts and shifts by the host word width.
                         */
                        if (a.op == 0x64 && shift && (value & 0x80000000UL))
                            bits |= (0xffffffffUL << (32 - shift)) & 0xffffffffUL;
                    }
                    bool unsigned_result = a.op == 0x65 &&
                        (!c->swf_loaded || c->swf_info.version < 8);
                    result = unsigned_result ? (double)bits : signed_bits(bits);
                }
                stack.pop_back();
                fvm_value_number(&stack.back(), result);
            }
            else if (a.op == 0x4e)
            {
                if (stack.size() < 2)
                    return FVM_ERROR;
                FVM_Value key, value;
                const FVM_Value &owner = stack[stack.size() - 2];
                const FVM_Value &member = stack.back();
                FVM_Result r;
                /* Legacy extension tables retain integer slots; standard/global members use string keys. */
                if (owner.type == FVM_VALUE_OBJECT && owner.object->resource_kind == 0 &&
                    owner.object != c->global && member.type == FVM_VALUE_NUMBER &&
                    member.number >= 0 && member.number <= 4294967295.0 &&
                    member.number == std::floor(member.number))
                    r = fvm_table_get_index(c, owner.object, (unsigned long)member.number, &value);
                else
                {
                    r = action_string(c, member, &key);
                    if (r == FVM_OK)
                        r = fvm_value_get_property_bytes(c, &owner, key.string, fvm_string_size(key), &value);
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
                if (owner.type == FVM_VALUE_OBJECT && owner.object->resource_kind == 0 &&
                    owner.object != c->global && member.type == FVM_VALUE_NUMBER &&
                    member.number >= 0 && member.number <= 4294967295.0 &&
                    member.number == std::floor(member.number))
                    r = fvm_table_set_index(c, owner.object, (unsigned long)member.number,
                                            &stack.back());
                else
                {
                    r = action_string(c, member, &key);
                    if (r == FVM_OK)
                        r = fvm_value_set_property_bytes(c, &owner, key.string, fvm_string_size(key), &stack.back());
                }
                if (r != FVM_OK)
                    return r;
                stack.resize(stack.size() - 3);
            }
            else if (a.op == 0x3a)
            { /* Delete: owner and property name; absent/inherited members report false. */
                if (stack.size() < 2)
                    return FVM_ERROR;
                /* [GC:] Retain owner and coercible name in the stack until deletion completes. */
                FVM_Value owner = stack[stack.size() - 2], key, result;
                FVM_Result status = action_string(c, stack.back(), &key);
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
                        /* Legacy Equals converts the top operand first; Less converts left first. */
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
                /* [GC:] The input remains in the rooted stack during toString execution. */
                FVM_Value text, ignored;
                FVM_Result status = stack.back().type == FVM_VALUE_UNDEFINED
                                        ? fvm_value_string(&text, "undefined")
                                        : action_string(c, stack.back(), &text);
                if (status != FVM_OK)
                    return status;
                FVM_NativeEntry sink = c->avm1_trace;
                if (sink.callback)
                {
                    fvm_value_undefined(&ignored);
                    /* [GC:] Prevent collection while the copied sink/capture and text are borrowed. */
                    FVM_NativeCallGuard guard(c);
                    status = sink.callback(c, c->global, 1, &text, &ignored, sink.userdata);
                    if (status != FVM_OK)
                        return status;
                }
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
                const FVM_Value &name_value = stack[a.op == 0x3c ? stack.size() - 2 : stack.size() - 1];
                FVM_Value text;
                FVM_Result status = action_string(c, name_value, &text);
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
                FVM_Result normalized = fvm_property_key(name_value.string, fvm_string_size(name_value), name);
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
                    r = captured ? fvm_object_set_bytes(c, captured, name.data(), name.size(), &stack.back())
                        : self && self->resource_kind == 40
                            ? fvm_object_set_bytes(c, self, name.data(), name.size(), &stack.back())
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
                /* [GC:] Both operands stay in the registered stack across hooks and coercion. */
                FVM_Value operands[2] = {stack[stack.size() - 2], stack.back()}, result;
                const char *hook = a.op == 0x0a   ? "__add"
                                   : a.op == 0x0b ? "__sub"
                                   : a.op == 0x0c ? "__mul"
                                                  : "__div";
                FVM_Result status = fvm_value_meta_invoke(c, operands, hook, 2, operands, &result);
                if (status == FVM_NOT_FOUND)
                    status = fvm_value_meta_invoke(c, operands + 1, hook, 2, operands, &result);
                if (status != FVM_OK && status != FVM_NOT_FOUND)
                    return status;
                if (status == FVM_NOT_FOUND)
                {
                    double left = 0, right = 0;
                    /* Subtract/Divide convert right first; Add/Multiply convert left first. */
                    if (a.op == 0x0b || a.op == 0x0d)
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
                    if (a.op == 0x0d && right == 0 && c->swf_loaded && c->swf_info.version < 5)
                        status = fvm_value_string(&result, "#ERROR#");
                    else
                        fvm_value_number(&result, a.op == 0x0a ? left + right
                                                   : a.op == 0x0b ? left - right
                                                   : a.op == 0x0c ? left * right
                                                                  : left / right);
                    if (status != FVM_OK)
                        return status;
                }
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
                    FVM_Result normalized = fvm_property_key(stack.back().string,
                                                               fvm_string_size(stack.back()), name);
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
                FVM_Result lookup =
                    fvm_lookup_native_method(receiver, name, &native_method);
                if (lookup != FVM_OK && lookup != FVM_NOT_FOUND && lookup != FVM_UNSUPPORTED)
                    return lookup;
                if (a.op == 0x53)
                {
                    FVM_Value function;
                    r = name.empty() ? FVM_OK
                                     : fvm_object_get_bytes(c, receiver, name.data(), name.size(), &function);
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
                    r = call_script(c, script->first, script->second, argc,
                                    argc ? &args[0] : 0, receiver, &result);
                else
                    r = fvm_object_call_bytes(c, receiver, name.data(), name.size(), argc, argc ? &args[0] : 0,
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
                fvm_detail::string name;
                FVM_Result normalized = fvm_property_key(stack.back().string, fvm_string_size(stack.back()), name);
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
                r = lookup_variable(c, locals, self, name, &callable, &defined,
                                    &call_receiver);
                if (r != FVM_OK)
                    return r;
                if (callable.type == FVM_VALUE_OBJECT && callable.object->invocable.callback)
                    r = fvm_function_invoke(c, callable.object, call_receiver, argc, args.begin(),
                                            &result);
                else if (defined)
                    r = FVM_UNSUPPORTED;
                else if (fn != c->script_functions.end())
                    r = call_script(c, fn->first, fn->second, argc, args.begin(), c->global,
                                    &result);
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
    FVM_Result r = fvm_avm1_inspect(data, size, &info);
    if (r != FVM_OK)
        return r;
    return execute(c, (const unsigned char *)data, size, 0, 0, c->global, 0);
}
/* [GC:] Replace a synchronous host trace sink after validating its retained capture. */
extern "C" FVM_Result fvm_avm1_set_trace_rooted(FVM_Context *c, FVM_NativeFunction callback,
                                                void *userdata, FVM_Object *capture)
{
    if (!c || c->destroying || c->gc_collecting ||
        (capture && capture->owner != c) || (!callback && (userdata || capture)))
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
    if (!c || c->destroying || c->gc_collecting ||
        (capture && capture->owner != c) || (!callback && (userdata || capture)))
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
        if (fvm_validate_value(c, args+i) != FVM_OK)
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
        return found == c->script_functions.end() ? FVM_NOT_FOUND
            : call_script(c, key, found->second, argc, args, self ? self : c->global, out);
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

/* Invoke avm1 script. */
FVM_Result fvm_avm1_invoke_script(FVM_Context *c, const FVM_ScriptFunction &body,
                                  FVM_Object *closure, FVM_Object *receiver, unsigned int argc,
                                  const FVM_Value *args, FVM_Value *out, FVM_Object *callee)
{
    return call_script(c, body.name.empty() ? fvm_detail::string("<function>") : body.name, body, argc, args,
                       receiver ? receiver : c->global, out, closure, callee);
}

/* Execute avm1 receiver. */
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
