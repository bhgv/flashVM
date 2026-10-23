/* FlashVM: TextField registration uses ordinary managed graph edges, never raw resource roots. */
#include "text_listeners.hpp"
#include "base_state.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/execution_work_scope.hpp"
#include "flashvm/property.h"
#include "../core/optional_method.hpp"

namespace
{
/* Reject lifecycle/version mismatches without invoking a property getter or user callback. */
FVM_Result usable(FVM_Context *context, FVM_Object *field)
{
    if (!context || !field || field->owner != context ||
        field->resource_kind != FVM_RESOURCE_TEXT_FIELD || !field->resource ||
        context->destroying || context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    return context->swf_loaded && context->swf_info.version < FVM_TEXT_LISTENER_MIN_VERSION
        ? FVM_UNSUPPORTED : FVM_OK;
}
}
/* Inspect only trusted native storage; malformed graph data cannot bypass count/owner bounds. */
FVM_Result fvm_textfield_listener_registry(FVM_Context *context, FVM_Object *field,
                                          FVM_Object **out)
{
    *out = 0;
    FVM_Result result = usable(context, field);
    if (result != FVM_OK)
        return result;
    fvm_detail::map<fvm_detail::string, FVM_Value>::iterator slot =
        field->properties.find(FVM_TEXT_LISTENER_KEY);
    if (slot == field->properties.end())
        return FVM_NOT_FOUND;
    if (slot->second.type != FVM_VALUE_OBJECT || !slot->second.object ||
        slot->second.object->owner != context)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *array = slot->second.object;
    if (array->resource_kind != FVM_RESOURCE_ARRAY || !array->resource_result_valid ||
        array->resource_result.type != FVM_VALUE_NUMBER ||
        array->indexed.size() > FVM_TEXT_LISTENER_MAX_COUNT ||
        array->resource_result.number != (double)array->indexed.size())
        return FVM_INVALID_ARGUMENT;
    FVM_ExecutionWorkScope work(context);
    unsigned long index = 0;
    for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin();
         i != array->indexed.end(); ++i, ++index)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        if (i->first != index || i->second.type != FVM_VALUE_OBJECT || !i->second.object ||
            i->second.object->owner != context)
            return FVM_INVALID_ARGUMENT;
    }
    *out = array;
    return FVM_OK;
}
/* [GC:] Arguments/receiver stay rooted by the method dispatcher; root new arrays until publication. */
FVM_Result fvm_textfield_add_listener(FVM_Context *context, FVM_Object *field, unsigned int count,
                                     const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_boolean(out, 0);
    FVM_Result result = usable(context, field);
    if (result != FVM_OK)
        return result;
    if (!count || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    if (args[0].object->owner != context)
        return FVM_INVALID_ARGUMENT;
    FVM_ExecutionWorkScope work(context);
    FVM_Object *array = 0;
    result = fvm_textfield_listener_registry(context, field, &array);
    if (result != FVM_OK && result != FVM_NOT_FOUND)
        return result;
    if (array)
    {
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin();
             i != array->indexed.end(); ++i)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            if (i->second.object == args[0].object)
            {
                fvm_value_boolean(out, 1);
                return FVM_OK;
            }
        }
        if (array->indexed.size() >= FVM_TEXT_LISTENER_MAX_COUNT)
            return FVM_ERROR;
        fvm_detail::map<unsigned long, FVM_Value> next;
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin();
             i != array->indexed.end(); ++i)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            next[i->first] = i->second;
        }
        result = work.step();
        if (result != FVM_OK)
            return result;
        next[(unsigned long)next.size()] = args[0];
        array->indexed.swap(next);
        fvm_value_number(&array->resource_result, (double)array->indexed.size());
    }
    else
    {
        result = fvm_array_create_like(context, 0, 0, &array, true);
        if (result != FVM_OK)
            return result;
        FVM_ObjectRootScope root(context, array);
        array->indexed[0] = args[0];
        fvm_value_number(&array->resource_result, 1);
        fvm_detail::map<fvm_detail::string, FVM_Value> properties;
        fvm_detail::map<fvm_detail::string, unsigned int> attributes;
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator i = field->properties.begin();
             i != field->properties.end(); ++i)
        {
            result = work.step();
            if (result != FVM_OK) return result;
            properties[i->first] = i->second;
        }
        for (fvm_detail::map<fvm_detail::string, unsigned int>::iterator i = field->attributes.begin();
             i != field->attributes.end(); ++i)
        {
            result = work.step();
            if (result != FVM_OK) return result;
            attributes[i->first] = i->second;
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = array;
        properties[FVM_TEXT_LISTENER_KEY] = value;
        attributes[FVM_TEXT_LISTENER_KEY] = FVM_PROPERTY_DONT_ENUM | FVM_PROPERTY_DONT_DELETE |
                                            FVM_PROPERTY_READ_ONLY;
        result = work.step();
        if (result != FVM_OK)
            return result;
        field->properties.swap(properties);
        field->attributes.swap(attributes);
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}
/* Stage a compact replacement before changing length; removals during delivery affect later events. */
FVM_Result fvm_textfield_remove_listener(FVM_Context *context, FVM_Object *field, unsigned int count,
                                        const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_boolean(out, 0);
    FVM_Result result = usable(context, field);
    if (result != FVM_OK)
        return result;
    if (!count || args[0].type != FVM_VALUE_OBJECT || !args[0].object)
        return FVM_OK;
    if (args[0].object->owner != context)
        return FVM_INVALID_ARGUMENT;
    FVM_ExecutionWorkScope work(context);
    FVM_Object *array = 0;
    result = fvm_textfield_listener_registry(context, field, &array);
    if (result == FVM_NOT_FOUND)
        return FVM_OK;
    if (result != FVM_OK)
        return result;
    unsigned long removed = (unsigned long)array->indexed.size();
    for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin();
         i != array->indexed.end(); ++i)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        if (i->second.object == args[0].object)
        {
            removed = i->first;
            break;
        }
    }
    if (removed == array->indexed.size())
        return FVM_OK;
    fvm_detail::map<unsigned long, FVM_Value> next;
    for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin();
         i != array->indexed.end(); ++i)
    {
        result = work.step();
        if (result != FVM_OK)
            return result;
        if (i->first != removed)
            next[i->first > removed ? i->first - 1 : i->first] = i->second;
    }
    array->indexed.swap(next);
    fvm_value_number(&array->resource_result, (double)array->indexed.size());
    fvm_value_boolean(out, 1);
    return FVM_OK;
}

/* [GC:] Field and vector roots remain linked across preparation, reentry and delivery failures. */
FVM_TextFieldScrollPlan::FVM_TextFieldScrollPlan(FVM_Context *context, FVM_Object *field)
    : context_(context), field_(field), field_root_(context, field),
      recipient_roots_(context, recipients_),
      enabled_(!context->swf_loaded || context->swf_info.version >= FVM_TEXT_LISTENER_MIN_VERSION),
      work_(context, enabled_), prepared_(false)
{
}
/* No handler lookup runs until the setter has published its new state. */
FVM_Result FVM_TextFieldScrollPlan::prepare()
{
    if (prepared_)
        return FVM_INVALID_ARGUMENT;
    if (enabled_)
    {
        FVM_Object *array = 0;
        FVM_Result result = fvm_textfield_listener_registry(context_, field_, &array);
        if (result != FVM_OK && result != FVM_NOT_FOUND)
            return result;
        if (array)
        {
            recipients_.reserve(array->indexed.size());
            for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin();
                 i != array->indexed.end(); ++i)
            {
                result = work_.step();
                if (result != FVM_OK)
                    return result;
                recipients_.push_back(i->second);
            }
        }
    }
    prepared_ = true;
    return FVM_OK;
}
/* Publication precedes callbacks; nested scroll mutations are never overwritten on return. */
FVM_Result FVM_TextFieldScrollPlan::dispatch()
{
    if (!prepared_)
        return FVM_INVALID_ARGUMENT;
    prepared_ = false;
    if (!enabled_)
        return FVM_OK;
    FVM_Value argument;
    fvm_value_undefined(&argument);
    argument.type = FVM_VALUE_OBJECT;
    argument.object = field_;
    FVM_Result result = work_.step();
    if (result == FVM_OK)
        result = fvm_object_call_optional_args(context_, field_, "onScroller", 1, &argument);
    for (size_t i = 0; result == FVM_OK && i < recipients_.size(); ++i)
    {
        result = work_.step();
        if (result == FVM_OK)
            result = fvm_object_call_optional_args(context_, recipients_[i].object,
                                                   "onScroller", 1, &argument);
    }
    return result;
}
