/* FlashVM: counted __resolve lookup and rooted, bounded legacy callback execution. */
#include "property_resolve.hpp"
#include "property_resolve_constants.h"
#include "property_lookup.hpp"
#include "object_root_scope.hpp"
#include "call_scope.hpp"
#include "../standard/base_state.hpp"
#include "../standard/function_state.hpp"
#include "flashvm/function.h"

namespace
{
/* Restore resolver recursion state on callback errors and allocation unwinding. */
class ResolveScope
{
    FVM_Context *context_; /* Context owning the active resolver depth. */
    /* A depth increment has unique stack ownership. */
    ResolveScope(const ResolveScope &);
    /* Assignment would duplicate the corresponding decrement. */
    ResolveScope &operator=(const ResolveScope &);
  public:
    /* Enter only after the caller has checked the recursion bound. */
    explicit ResolveScope(FVM_Context *context) : context_(context) { ++context_->resolve_depth; }
    /* Restore the caller's depth on every return path. */
    ~ResolveScope() { --context_->resolve_depth; }
};
}

/* Find the first Object-valued raw resolver; primitive entries do not shadow ancestors. */
FVM_Result fvm_property_resolve(FVM_Context *c, FVM_Object *start, FVM_Object *receiver,
                                const fvm_detail::string &name, FVM_Value *out, bool *handled)
{
    if (!handled)
        return FVM_INVALID_ARGUMENT;
    *handled = false;
    if (!c || !receiver || receiver->owner != c || !out || (start && start->owner != c))
        return FVM_INVALID_ARGUMENT;
    if (!c->swf_loaded || !start)
        return FVM_OK;
    /* [GC:] Prototype mutation inside a resolver must not invalidate the original lookup graph. */
    FVM_ObjectRootScope lookup_roots(c, start);
    FVM_ObjectRootScope receiver_roots(c, receiver);
    const fvm_detail::string key(FVM_PROPERTY_RESOLVE_NAME);
    FVM_Object *owner = start;
    for (unsigned int depth = 0; owner && depth < FVM_PROTOTYPE_MAX_DEPTH; ++depth)
    {
        if (!fvm_property_available(c, owner, key))
        {
            owner = owner->prototype;
            continue;
        }
        FVM_Value resolver;
        fvm_value_undefined(&resolver);
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator data =
            fvm_property_find(c, owner->properties, key);
        if (data != owner->properties.end())
            resolver = data->second;
        else
        {
            fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator accessor =
                fvm_property_find(c, owner->accessors, key);
            if (accessor != owner->accessors.end())
                fvm_base_accessor_data(accessor->second, resolver);
            else
            {
                fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method =
                    fvm_property_find(c, owner->methods, key);
                if (method != owner->methods.end())
                {
                    FVM_Result status = fvm_function_wrap_entry(c, owner, method->first,
                                                                 method->second, &resolver);
                    if (status != FVM_OK)
                        return status;
                }
            }
        }
        if (resolver.type == FVM_VALUE_OBJECT)
        {
            *handled = true;
            fvm_value_undefined(out);
            /* A non-Function Object stops resolver search but calling it produces undefined. */
            if (!resolver.object->invocable.callback)
                return FVM_OK;
            if (c->resolve_depth >= FVM_PROPERTY_RESOLVE_MAX_DEPTH)
                return FVM_ERROR;
            ResolveScope depth_scope(c);
            FVM_Value argument;
            FVM_Result status = fvm_value_string_bytes(&argument, name.c_str(), name.size());
            if (status != FVM_OK)
                return status;
            /* [GC:] Resolver calls have no selected member owner; preserve the caller's identity. */
            FVM_CallScope call(c, 0, resolver.object, 0);
            FVM_Value result;
            fvm_value_undefined(&result);
            status = fvm_function_invoke(c, resolver.object, receiver, 1, &argument, &result);
            /* Publish only a completed result; failed string snapshots may still point into name. */
            if (status == FVM_OK)
                *out = result;
            return status;
        }
        owner = owner->prototype;
    }
    return owner ? FVM_ERROR : FVM_OK;
}
