#include "../core/native_call_guard.hpp"
#include "flashvm/stack.h"
#include "flashvm/auxlib.h"
#include <climits>
#include "flashvm/callable.h"
#include "../core/runtime.hpp"
#include <new>
#include <cstring>
struct FVM_Stack
{
    FVM_Context *context;
    fvm_detail::vector<FVM_Value> values;
};
static int valid(FVM_Context *c, const FVM_Value *v)
{
    return fvm_validate_value(c, v) == FVM_OK;
}
extern "C"
{
    FVM_Result fvm_stack_create(FVM_Context *c, FVM_Stack **out)
    {
        if (!c || !out)
            return FVM_INVALID_ARGUMENT;
        *out = new (std::nothrow) FVM_Stack;
        if (!*out)
            return FVM_OUT_OF_MEMORY;
        (*out)->context = c;
        try
        {
            c->gc_stacks.push_back(&(*out)->values);
        }
        catch (...)
        {
            delete *out;
            *out = 0;
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    void fvm_stack_destroy(FVM_Stack *s)
    {
        if (!s)
            return;
        for (size_t i = 0; i < s->context->gc_stacks.size(); ++i)
            if (s->context->gc_stacks[i] == &s->values)
            {
                s->context->gc_stacks.erase(s->context->gc_stacks.begin() + i);
                break;
            }
        delete s;
    }
    unsigned int fvm_stack_size(const FVM_Stack *s)
    {
        return s ? (unsigned int)s->values.size() : 0;
    }
    FVM_Result fvm_stack_get(const FVM_Stack *s, int index, FVM_Value *out)
    {
        long n, pos;
        if (!s || !out || index == 0)
            return FVM_INVALID_ARGUMENT;
        n = (long)s->values.size();
        pos = index > 0 ? (long)index - 1 : n + (long)index;
        if (pos < 0 || pos >= n)
            return FVM_INVALID_ARGUMENT;
        *out = s->values[(size_t)pos];
        return FVM_OK;
    }
    FVM_Result fvm_stack_push(FVM_Stack *s, const FVM_Value *v)
    {
        if (!s || !valid(s->context, v))
            return FVM_INVALID_ARGUMENT;
        try
        {
            s->values.push_back(*v);
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    FVM_Result fvm_stack_pop(FVM_Stack *s, unsigned int count)
    {
        if (!s || count > s->values.size())
            return FVM_INVALID_ARGUMENT;
        while (count--)
            s->values.pop_back();
        return FVM_OK;
    }
    FVM_Result fvm_stack_clear(FVM_Stack *s)
    {
        if (!s)
            return FVM_INVALID_ARGUMENT;
        return fvm_stack_pop(s, (unsigned int)s->values.size());
    }
    FVM_Result fvm_stack_register(FVM_Context *c, const char *name, FVM_StackFunction cb,
                                  void *user)
    {
        FVM_StackEntry e;
        if (!c || !name || !*name || !cb)
            return FVM_INVALID_ARGUMENT;
        e.callback = cb;
        e.userdata = user;
        try
        {
            c->stack_functions[name] = e;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    FVM_Result fvm_stack_call(FVM_Context *c, const char *name, unsigned int argc,
                              const FVM_Value *args, FVM_Value *out)
    {
        FVM_Stack *s = 0;
        FVM_Result r;
        unsigned int i;
        if (!c || !name || !out || (argc && !args))
            return FVM_INVALID_ARGUMENT;
        fvm_detail::map<fvm_detail::string, FVM_StackEntry>::iterator it =
            c->stack_functions.find(name);
        if (it == c->stack_functions.end())
            return FVM_UNSUPPORTED;
        /* Copy the entry before user code can replace its registration. */
        FVM_StackEntry entry = it->second;
        r = fvm_stack_create(c, &s);
        if (r != FVM_OK)
            return r;
        for (i = 0; i < argc; ++i)
        {
            r = fvm_stack_push(s, &args[i]);
            if (r != FVM_OK)
                break;
        }
        if (r == FVM_OK)
        {
            try
            {
                FVM_NativeCallGuard native_guard(c);
                if (entry.cfunction)
                {
                    int count = entry.cfunction(s);
                    if (count < 0)
                        r = (FVM_Result)count;
                    else if (count > 1 || (size_t)count > s->values.size())
                        r = FVM_ERROR;
                    else if (count == 0)
                        s->values.clear();
                    else
                    {
                        FVM_Value value = s->values.back();
                        s->values.clear();
                        s->values.push_back(value);
                    }
                }
                else
                    r = entry.callback(c, s, entry.userdata);
            }
            catch (...)
            {
                r = FVM_ERROR;
            }
        }
        if (r == FVM_OK)
        {
            if (s->values.size() > 1)
                r = FVM_ERROR;
            else if (s->values.size() == 1)
                *out = s->values[0];
            else
                fvm_value_undefined(out);
        }
        fvm_stack_destroy(s);
        return r;
    }
}

extern "C"
{
    FVM_Result fvm_callable_create(FVM_Context *c, FVM_StackFunction cb, void *user,
                                   FVM_Object **out)
    {
        FVM_Object *obj = 0;
        FVM_StackEntry entry;
        FVM_Result r;
        if (!c || !cb || !out)
            return FVM_INVALID_ARGUMENT;
        *out = 0;
        r = fvm_object_create(c, &obj);
        if (r != FVM_OK)
            return r;
        entry.callback = cb;
        entry.userdata = user;
        obj->callable = entry;
        *out = obj;
        return FVM_OK;
    }
    FVM_Result fvm_callable_is(FVM_Context *c, FVM_Object *obj, int *out)
    {
        if (!c || !obj || !out || obj->owner != c)
            return FVM_INVALID_ARGUMENT;
        *out = obj->callable.callback != 0;
        return FVM_OK;
    }
    FVM_Result fvm_callable_invoke(FVM_Context *c, FVM_Object *obj, unsigned int argc,
                                   const FVM_Value *args, FVM_Value *out)
    {
        FVM_Stack *stack = 0;
        FVM_Result r;
        unsigned int i;
        if (!c || !obj || !out || obj->owner != c || (argc && !args))
            return FVM_INVALID_ARGUMENT;
        FVM_StackEntry entry = obj->callable;
        if (!entry.callback)
            return FVM_UNSUPPORTED;
        r = fvm_stack_create(c, &stack);
        if (r != FVM_OK)
            return r;
        for (i = 0; i < argc; ++i)
        {
            r = fvm_stack_push(stack, &args[i]);
            if (r != FVM_OK)
                break;
        }
        if (r == FVM_OK)
        {
            try
            {
                FVM_NativeCallGuard native_guard(c);
                r = entry.callback(c, stack, entry.userdata);
            }
            catch (...)
            {
                r = FVM_ERROR;
            }
        }
        if (r == FVM_OK)
        {
            if (stack->values.size() > 1)
                r = FVM_ERROR;
            else if (stack->values.size() == 1)
                *out = stack->values[0];
            else
                fvm_value_undefined(out);
        }
        fvm_stack_destroy(stack);
        return r;
    }
}

extern "C" FVM_Result fvm_stack_register_rooted(FVM_Context *c, const char *name,
                                                FVM_StackFunction cb, void *ud, FVM_Object *root)
{
    if (!c || (root && root->owner != c))
        return FVM_INVALID_ARGUMENT;
    FVM_Result r = fvm_stack_register(c, name, cb, ud);
    if (r == FVM_OK)
        c->stack_functions.find(name)->second.root = root;
    return r;
}
extern "C" FVM_Result fvm_callable_create_rooted(FVM_Context *c, FVM_StackFunction cb, void *ud,
                                                 FVM_Object *root, FVM_Object **out)
{
    if (!c || (root && root->owner != c))
        return FVM_INVALID_ARGUMENT;
    FVM_Result r = fvm_callable_create(c, cb, ud, out);
    if (r == FVM_OK)
        (*out)->callable.root = root;
    return r;
}

namespace
{
/* Convert signed public indices without overflowing on INT_MIN. */
FVM_Result stack_position(FVM_Stack *stack, int index, size_t *position)
{
    if (!stack || !position || index == 0)
        return FVM_INVALID_ARGUMENT;
    size_t count = stack->values.size();
    if (index > 0)
    {
        *position = (size_t)index - 1;
        return *position < count ? FVM_OK : FVM_INVALID_ARGUMENT;
    }
    size_t distance = (size_t)(-(index + 1)) + 1;
    if (distance > count)
        return FVM_INVALID_ARGUMENT;
    *position = count - distance;
    return FVM_OK;
}
} // namespace

extern "C"
{
    FVM_Context *fvm_stack_context(FVM_Stack *stack)
    {
        return stack ? stack->context : 0;
    }

    FVM_Result fvm_stack_register_cfunction(FVM_Context *context, const char *name,
                                            FVM_CFunction callback, FVM_Object *root)
    {
        if (!context || !name || !*name || !callback || (root && root->owner != context))
            return FVM_INVALID_ARGUMENT;
        FVM_StackEntry entry;
        entry.cfunction = callback;
        entry.root = root;
        try
        {
            context->stack_functions[name] = entry;
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }

    FVM_Result fvm_stack_settop(FVM_Stack *stack, int index)
    {
        if (!stack)
            return FVM_INVALID_ARGUMENT;
        size_t count = stack->values.size();
        size_t target;
        if (index >= 0)
            target = (size_t)index;
        else
        {
            size_t distance = (size_t)(-(index + 1));
            if (distance > count)
                return FVM_INVALID_ARGUMENT;
            target = count - distance;
        }
        try
        {
            /* Default C++ construction does not initialize FVM_Value PODs. */
            stack->values.reserve(target);
            while (stack->values.size() < target)
            {
                FVM_Value value;
                fvm_value_undefined(&value);
                stack->values.push_back(value);
            }
            while (stack->values.size() > target)
                stack->values.pop_back();
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }

    FVM_Result fvm_stack_pushvalue(FVM_Stack *stack, int index)
    {
        FVM_Value value;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        return result == FVM_OK ? fvm_stack_push(stack, &value) : result;
    }

    FVM_Result fvm_stack_remove(FVM_Stack *stack, int index)
    {
        size_t position;
        FVM_Result result = stack_position(stack, index, &position);
        if (result != FVM_OK)
            return result;
        stack->values.erase(stack->values.begin() + position);
        return FVM_OK;
    }

    FVM_Result fvm_stack_insert(FVM_Stack *stack, int index)
    {
        size_t position;
        FVM_Result result = stack_position(stack, index, &position);
        if (result != FVM_OK)
            return result;
        FVM_Value top = stack->values.back();
        for (size_t i = stack->values.size() - 1; i > position; --i)
            stack->values[i] = stack->values[i - 1];
        stack->values[position] = top;
        return FVM_OK;
    }

    FVM_Result fvm_stack_replace(FVM_Stack *stack, int index)
    {
        size_t position;
        FVM_Result result = stack_position(stack, index, &position);
        if (result != FVM_OK)
            return result;
        stack->values[position] = stack->values.back();
        stack->values.pop_back();
        return FVM_OK;
    }

    FVM_Result fvm_stack_pushnumber(FVM_Stack *stack, double number)
    {
        FVM_Value value;
        fvm_value_number(&value, number);
        return fvm_stack_push(stack, &value);
    }

    FVM_Result fvm_stack_pushboolean(FVM_Stack *stack, int boolean)
    {
        FVM_Value value;
        fvm_value_boolean(&value, boolean);
        return fvm_stack_push(stack, &value);
    }

    FVM_Result fvm_stack_pushundefined(FVM_Stack *stack)
    {
        FVM_Value value;
        fvm_value_undefined(&value);
        return fvm_stack_push(stack, &value);
    }

    FVM_Result fvm_stack_pushstring(FVM_Stack *stack, const char *text)
    {
        if (!stack || !text)
            return FVM_INVALID_ARGUMENT;
        size_t length = std::strlen(text);
        if (length == (size_t)-1)
            return FVM_OUT_OF_MEMORY;
        char *copy = new (std::nothrow) char[length + 1];
        if (!copy)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(copy, text, length + 1);
        try
        {
            stack->context->strings.push_back(copy);
        }
        catch (...)
        {
            delete[] copy;
            return FVM_OUT_OF_MEMORY;
        }
        FVM_Value value;
        fvm_value_string(&value, copy);
        return fvm_stack_push(stack, &value);
    }

    FVM_Result fvm_stack_checknumber(FVM_Stack *stack, int index, double *out)
    {
        FVM_Value value;
        if (!out)
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        if (result != FVM_OK)
            return result;
        if (value.type != FVM_VALUE_NUMBER)
            return FVM_INVALID_ARGUMENT;
        *out = value.number;
        return FVM_OK;
    }

    FVM_Result fvm_stack_checkstring(FVM_Stack *stack, int index, const char **out)
    {
        FVM_Value value;
        if (!out)
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_stack_get(stack, index, &value);
        if (result != FVM_OK)
            return result;
        if (value.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        *out = value.string;
        return FVM_OK;
    }

    FVM_Result fvm_stack_newtable(FVM_Stack *stack)
    {
        if (!stack)
            return FVM_INVALID_ARGUMENT;
        FVM_Object *object = 0;
        FVM_Result result = fvm_object_create(stack->context, &object);
        if (result != FVM_OK)
            return result;
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = object;
        return fvm_stack_push(stack, &value);
    }

    FVM_Result fvm_stack_getfield(FVM_Stack *stack, int index, const char *name)
    {
        FVM_Value object, value;
        FVM_Result result = fvm_stack_get(stack, index, &object);
        if (result != FVM_OK)
            return result;
        if (object.type != FVM_VALUE_OBJECT || !name)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_get(stack->context, object.object, name, &value);
        return result == FVM_OK ? fvm_stack_push(stack, &value) : result;
    }

    FVM_Result fvm_stack_setfield(FVM_Stack *stack, int index, const char *name)
    {
        FVM_Value object;
        FVM_Result result = fvm_stack_get(stack, index, &object);
        if (result != FVM_OK)
            return result;
        if (object.type != FVM_VALUE_OBJECT || !name)
            return FVM_INVALID_ARGUMENT;
        result = fvm_object_set(stack->context, object.object, name, &stack->values.back());
        if (result == FVM_OK)
            stack->values.pop_back();
        return result;
    }

    FVM_Result fvm_stack_getglobal(FVM_Stack *stack, const char *name)
    {
        if (!stack || !name)
            return FVM_INVALID_ARGUMENT;
        FVM_Value value;
        FVM_Result result = fvm_get_global(stack->context, name, &value);
        return result == FVM_OK ? fvm_stack_push(stack, &value) : result;
    }

    FVM_Result fvm_stack_setglobal(FVM_Stack *stack, const char *name)
    {
        if (!stack || !name || stack->values.empty())
            return FVM_INVALID_ARGUMENT;
        FVM_Result result = fvm_set_global(stack->context, name, &stack->values.back());
        if (result == FVM_OK)
            stack->values.pop_back();
        return result;
    }
}
