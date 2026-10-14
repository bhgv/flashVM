/* FlashVM: dynamic addition, equality and ordering with all supported SWF policies. */
#include "support.hpp"
#include "../../standard/string_order.hpp"
#include "../../standard/primitive_state.hpp"
#include <cstring>
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
    FVM_Context *context;                 /* Context owning this temporary root frame. */
    FVM_GCRootFrame frame;                /* [GC:] Frame linked above the caller's operand roots. */
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
        result = strict_equal(a, b) || (c->swf_loaded && a.type == FVM_VALUE_NUMBER &&
                                        a.number != a.number && b.number != b.number);
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

/* Execute dynamic addition/comparison without instruction or operand-stack dependencies. */
extern "C" FVM_Result fvm_exec_compare(FVM_Context *c, FVM_ExecutionCompare operation,
                                       const FVM_Value *left_value, const FVM_Value *right_value,
                                       FVM_Value *out)
{
    if (!c || !out || operation < FVM_EXEC_ADD_DYNAMIC || operation > FVM_EXEC_GREATER ||
        fvm_validate_value(c, left_value) != FVM_OK || fvm_validate_value(c, right_value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        if (operation == FVM_EXEC_STRICT_EQUAL)
        {
            fvm_value_boolean(out, strict_equal(*left_value, *right_value));
            return FVM_OK;
        }
        /* [GC:] Retain transformed primitives and derived strings across subsequent callbacks. */
        FVM_ExecutionRoots roots(c, 4);
        fvm_detail::vector<FVM_Value> &converted = roots.values;
        converted[0] = *left_value;
        converted[1] = *right_value;
        fvm_value_undefined(&converted[2]);
        fvm_value_undefined(&converted[3]);
        FVM_Value &left = converted[0], &right = converted[1];
        FVM_Value output;
        FVM_Result r = FVM_OK;
        if (operation != FVM_EXEC_STRICT_EQUAL)
        {
            const char *hook = operation == FVM_EXEC_ADD_DYNAMIC ? "__add"
                               : operation == FVM_EXEC_LESS      ? "__lt"
                               : operation == FVM_EXEC_GREATER   ? "__gt"
                                                                 : "__eq";
            FVM_Value operands[2] = {left, right};
            r = fvm_value_meta_invoke(c, operands, hook, 2, operands, &output);
            if (r == FVM_NOT_FOUND)
                r = fvm_value_meta_invoke(c, operands + 1, hook, 2, operands, &output);
            if (r == FVM_OK)
            {
                if (operation != FVM_EXEC_ADD_DYNAMIC)
                    fvm_value_boolean(&output, truth(output));
                *out = output;
                return FVM_OK;
            }
            if (r != FVM_NOT_FOUND)
                return r;
            r = FVM_OK;
        }
        if (operation == FVM_EXEC_EQUAL)
        {
            bool matches = false;
            r = equal(c, left, right, matches);
            fvm_value_boolean(&output, matches);
        }
        else
        {
            if (operation == FVM_EXEC_ADD_DYNAMIC)
            {
                r = fvm_avm1_primitive(c, &right, true, &right);
                if (r == FVM_OK)
                    r = fvm_avm1_primitive(c, &left, true, &left);
            }
            else
            {
                FVM_Value &first = operation == FVM_EXEC_GREATER ? right : left;
                FVM_Value &second = operation == FVM_EXEC_GREATER ? left : right;
                r = fvm_avm1_primitive(c, &first, false, &first);
                if (r == FVM_OK && c->swf_loaded && first.type == FVM_VALUE_OBJECT &&
                    first.object->resource_kind != 40)
                {
                    fvm_value_boolean(out, 0);
                    return FVM_OK;
                }
                if (r == FVM_OK)
                    r = fvm_avm1_primitive(c, &second, false, &second);
                if (r == FVM_OK && c->swf_loaded && second.type == FVM_VALUE_OBJECT &&
                    second.object->resource_kind != 40)
                {
                    fvm_value_boolean(out, 0);
                    return FVM_OK;
                }
            }
            if (r == FVM_OK && operation == FVM_EXEC_ADD_DYNAMIC &&
                (left.type == FVM_VALUE_STRING || right.type == FVM_VALUE_STRING))
            {
                FVM_Value &x = converted[2], &y = converted[3];
                r = fvm_avm1_string(c, left, &x);
                if (r == FVM_OK)
                    r = fvm_avm1_string(c, right, &y);
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
            else if (r == FVM_OK && (operation == FVM_EXEC_LESS || operation == FVM_EXEC_GREATER) &&
                     left.type == FVM_VALUE_STRING && right.type == FVM_VALUE_STRING)
            {
                int order = 0;
                r = fvm_string_order(left, right, order);
                fvm_value_boolean(&output, operation == FVM_EXEC_GREATER ? order > 0 : order < 0);
            }
            else if (r == FVM_OK)
            {
                double x = 0, y = 0;
                r = fvm_avm1_number(c, &left, &x);
                if (r == FVM_OK)
                    r = fvm_avm1_number(c, &right, &y);
                if (operation == FVM_EXEC_ADD_DYNAMIC)
                    fvm_value_number(&output, x + y);
                else if (x != x || y != y)
                    fvm_value_undefined(&output);
                else
                    fvm_value_boolean(&output, operation == FVM_EXEC_GREATER ? x > y : x < y);
            }
        }
        if (r != FVM_OK)
            return r;
        *out = output;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
