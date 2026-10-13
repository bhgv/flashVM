/* FlashVM: BlurFilter constructor, independent clones and atomic bounded parameter updates. */
#include "blur.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
namespace
{
using namespace fvm_geometry_detail;
const char *const names[] = {"blurX", "blurY", "quality"};
/* Normalize finite numeric options to documented blur/quality ranges before private publication. */
FVM_Result option(FVM_Context *c, unsigned index, const FVM_Value &value, double &out)
{
    double number;
    FVM_Result result = fvm_value_to_number(c, &value, &number);
    if (result != FVM_OK)
        return result;
    if (!(number >= -1.7976931348623157e308 && number <= 1.7976931348623157e308))
        return FVM_INVALID_ARGUMENT;
    double maximum = index == 2 ? 15 : 255;
    out = number < 0 ? 0 : number > maximum ? maximum : number;
    if (index == 2)
        out = std::floor(out);
    return FVM_OK;
}
/* [GC:] Allocate unpublished private numeric state and replace its owning edge only on success. */
FVM_Result store(FVM_Context *c, FVM_Object *self, const double *values)
{
    FVM_Object *state = 0;
    /* [GC:] Keep new private state protected by the caller's native barrier. */
    FVM_Result result = fvm_object_create(c, &state);
    if (result != FVM_OK)
        return result;
    for (unsigned i = 0; i < 3; ++i)
        state->properties[names[i]] = scalar(values[i]);
    /* [GC:] Publish a complete replacement and allow the previous data node to be collected. */
    self->resource_result = managed(state);
    /* [GC:] Enable tracing of the newly installed private data edge. */
    self->resource_result_valid = true;
    return FVM_OK;
}
/* [GC:] Construct an instance using its captured factory and independently allocated options. */
FVM_Result make(FVM_Context *c, void *u, const double *values, FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result result = captured_class(c, u, "BlurFilter", klass);
    if (result == FVM_OK)
        result = create(c, klass, FVM_BLUR_FILTER, 0, 0, 0, 0, out);
    if (result == FVM_OK)
        result = store(c, out->object, values);
    if (result != FVM_OK)
        fvm_value_undefined(out);
    return result;
}
/* [GC:] Convert constructor arguments before allocating the class instance and private options. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args,
                       FVM_Value *out, void *u)
{
    /* [GC:] Protect public coercion results and all unpublished constructor allocations. */
    FVM_NativeCallGuard guard(c);
    double values[] = {4, 4, 1};
    FVM_Result result = FVM_OK;
    for (unsigned i = 0; result == FVM_OK && i < 3 && i < n; ++i)
        result = option(c, i, args[i], values[i]);
    return result == FVM_OK ? make(c, u, values, out) : result;
}
/* [GC:] Clone private parameters into independent storage using the retained constructor graph. */
FVM_Result clone(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out,
                 void *u)
{
    /* [GC:] Protect source, class and newly allocated clone state until publication. */
    FVM_NativeCallGuard guard(c);
    double values[3];
    fvm_value_undefined(out);
    return fvm_blur_values(c, self, values) ? make(c, u, values, out) : FVM_OK;
}
/* Return one copied private parameter without exposing its managed state node. */
template <unsigned Index>
/* Copy the selected bounded numeric property from protected private storage. */
FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out,
               void *)
{
    double values[3];
    fvm_value_undefined(out);
    if (fvm_blur_values(c, self, values))
        fvm_value_number(out, values[Index]);
    return FVM_OK;
}
/* [GC:] Convert one public replacement and atomically publish a refreshed complete parameter set.
 */
template <unsigned Index>
/* [GC:] Coerce the selected option and publish refreshed private values atomically. */
FVM_Result put(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out,
               void *)
{
    /* [GC:] Protect owning instance and conversion results through setter callbacks. */
    FVM_NativeCallGuard guard(c);
    double values[3], replacement;
    fvm_value_undefined(out);
    if (!n || !fvm_blur_values(c, self, values))
        return FVM_OK;
    FVM_Result result = option(c, Index, args[0], replacement);
    if (result != FVM_OK)
        return result;
    /* Conversion may reenter another setter; refresh unrelated options instead of losing its edit.
     */
    if (!fvm_blur_values(c, self, values))
        return FVM_INVALID_ARGUMENT;
    values[Index] = replacement;
    return store(c, self, values);
}
const FVM_NativeFunction callbacks[] = {constructor, clone,  get<0>, get<1>,
                                        get<2>,      put<0>, put<1>, put<2>};
} // namespace
/* [GC:] Verify and copy exactly three canonical private numbers without callback dispatch. */
bool fvm_blur_values(FVM_Context *c, FVM_Object *self, double *values)
{
    using namespace fvm_geometry_detail;
    if (!self || self->owner != c || self->resource_kind != FVM_BLUR_FILTER ||
        !self->resource_result_valid || self->resource_result.type != FVM_VALUE_OBJECT)
        return false;
    FVM_Object *state = self->resource_result.object;
    if (!state || state->owner != c || state->resource_kind || state->properties.size() != 3)
        return false;
    for (unsigned i = 0; i < 3; ++i)
    {
        Properties::iterator p = state->properties.find(names[i]);
        if (p == state->properties.end() || p->second.type != FVM_VALUE_NUMBER ||
            !(p->second.number >= 0 && p->second.number <= (i == 2 ? 15 : 255)) ||
            (i == 2 && p->second.number != std::floor(p->second.number)))
            return false;
        values[i] = p->second.number;
    }
    return true;
}
/* [GC:] Build BlurFilter inheritance and all captured callbacks in the unpublished namespace. */
FVM_Result fvm_blur_stage(FVM_Context *c, FVM_Object *space, FVM_Object *bindings,
                          FVM_Object *function_proto,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    using namespace fvm_geometry_detail;
    /* [GC:] Protect newly staged class, prototype and factory relationships. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *base = 0, *klass = 0, *prototype = 0;
    FVM_Value parent;
    FVM_Result result = captured_class(c, bindings, "BitmapFilter", base);
    if (result == FVM_OK)
        result = fvm_object_get(c, base, "prototype", &parent);
    if (result != FVM_OK || parent.type != FVM_VALUE_OBJECT)
        return result != FVM_OK ? result : FVM_INVALID_ARGUMENT;
    /* [GC:] Allocate the independent constructor identity. */
    result = fvm_object_create(c, &klass);
    if (result == FVM_OK)
        /* [GC:] Allocate its separately managed instance prototype. */
        result = fvm_object_create(c, &prototype);
    if (result != FVM_OK)
        return result;
    klass->resource_kind = FVM_BLUR_FILTER_CLASS;
    /* [GC:] Preserve native Function and BitmapFilter inheritance. */
    klass->prototype = function_proto;
    /* [GC:] Keep the retained base instance prototype reachable from blur instances. */
    prototype->prototype = parent.object;
    /* [GC:] Retain the instance prototype through the constructor. */
    klass->properties["prototype"] = managed(prototype);
    /* [GC:] Preserve reverse constructor identity for cloning and instanceof. */
    prototype->properties["constructor"] = managed(klass);
    prototype->attributes["constructor"] = 3;
    FVM_NativeEntry entry;
    entry.callback = constructor;
    /* [GC:] Retain the shared factory graph for every callback. */
    entry.root = bindings;
    /* [GC:] Userdata aliases that traced root. */
    entry.userdata = bindings;
    klass->constructable = klass->invocable = entry;
    constructors["flash.filters.BlurFilter"] = entry;
    entry.callback = clone;
    prototype->methods["clone"] = entry;
    prototype->attributes["clone"] = 3;
    FVM_AccessorEntry accessor;
    /* [GC:] Retain factories while executing imported accessor callbacks. */
    accessor.root = bindings;
    /* [GC:] Accessor userdata shares the same traced factory graph. */
    accessor.userdata = bindings;
    for (unsigned i = 0; i < 3; ++i)
    {
        accessor.getter = callbacks[2 + i];
        accessor.setter = callbacks[5 + i];
        prototype->accessors[names[i]] = accessor;
        prototype->attributes[names[i]] = 3;
    }
    /* [GC:] Publish the completed class into the staged namespace. */
    space->properties["BlurFilter"] = managed(klass);
    /* [GC:] Retain the same constructor identity in the captured factory graph. */
    bindings->properties["BlurFilter"] = managed(klass);
    return FVM_OK;
}
/* Validate the native constructor identity without public namespace reads. */
bool fvm_blur_validate(FVM_Context *c, FVM_Object *space)
{
    using namespace fvm_geometry_detail;
    if (!space || space->owner != c)
        return false;
    Properties::iterator p = space->properties.find("BlurFilter");
    return p != space->properties.end() && p->second.type == FVM_VALUE_OBJECT && p->second.object &&
           p->second.object->owner == c &&
           p->second.object->resource_kind == FVM_BLUR_FILTER_CLASS &&
           p->second.object->constructable.callback == constructor;
}
/* Map recognized blur constructor/clone/property functions to append-only transport IDs. */
unsigned long fvm_blur_callback_id(FVM_NativeFunction fn)
{
    for (unsigned i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i)
        if (callbacks[i] == fn)
            return 1650 + i;
    return 0;
}
/* Restore a known blur callback without requiring a global standard library installation. */
FVM_NativeFunction fvm_blur_callback_from_id(unsigned long id)
{
    return id >= 1650 && id < 1650 + sizeof(callbacks) / sizeof(callbacks[0]) ? callbacks[id - 1650]
                                                                              : 0;
}
