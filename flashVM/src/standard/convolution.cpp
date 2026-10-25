/* FlashVM: managed ConvolutionFilter coefficients, atomic property updates and edge sampling. */
#include "convolution.hpp"
#include "base_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include <cmath>

namespace
{
using namespace fvm_geometry_detail;
const char *const names[] = {"matrixX", "matrixY", "divisor", "bias", "preserveAlpha", "clamp", "color", "alpha"};


/* Convert a bounded kernel index to its public decimal property name without platform formatting.
 */
void key(unsigned index, char *text)
{
    unsigned position = 0;
    if (index >= 100)
    {
        text[position++] = (char)('0' + index / 100);
    }
    if (index >= 10)
    {
        text[position++] = (char)('0' + (index / 10) % 10);
    }
    text[position++] = (char)('0' + index % 10);
    text[position] = 0;
}


/* Test the finite coefficient safety bound before storing or evaluating host floating arithmetic.
 */
bool coefficient_valid(double number)
{
    return number >= -1000000.0 && number <= 1000000.0;
}


/* Convert numeric integer options using unsigned modular arithmetic rather than undefined casts. */
unsigned long word(double number)
{
    if (!(number == number && std::fabs(number) <= 1.7976931348623157e308))
    {
        return 0;
    }
    number = std::fmod(number < 0 ? std::ceil(number) : std::floor(number), 4294967296.0);
    return (unsigned long)(number < 0 ? number + 4294967296.0 : number);
}


/* Apply ActionScript boolean truth without invoking numeric conversion hooks. */
bool truth(const FVM_Value &value)
{
    return value.type == FVM_VALUE_OBJECT || (value.type == FVM_VALUE_BOOLEAN && value.boolean) ||
           (value.type == FVM_VALUE_NUMBER && value.number && value.number == value.number) ||
           (value.type == FVM_VALUE_STRING && fvm_value_string_size(&value));
}


/* Normalize one copied option while propagating public numeric conversion failures. */
FVM_Result option(FVM_Context *c, unsigned index, const FVM_Value &value, double &out)
{
    if (index == FVM_CONVOLUTION_PRESERVE || index == FVM_CONVOLUTION_CLAMP)
    {
        out = truth(value) ? 1 : 0;
        return FVM_OK;
    }
    double number = 0;
    FVM_Result r = fvm_value_to_number(c, &value, &number);
    if (r != FVM_OK)
    {
        return r;
    }
    if (index == FVM_CONVOLUTION_X || index == FVM_CONVOLUTION_Y)
    {
        unsigned long integer = word(number);
        double signed_integer = integer >= 0x80000000UL ? (double)integer - 4294967296.0 : (double)integer;
        out = signed_integer < 0 ? 0 : signed_integer > 15 ? 15 : signed_integer;
    }
    else if (index == FVM_CONVOLUTION_COLOR)
    {
        out = word(number) & 0xffffffUL;
    }
    else
    {
        if (!coefficient_valid(number))
        {
            return FVM_INVALID_ARGUMENT;
        }
        out = index == FVM_CONVOLUTION_ALPHA ? (number < 0 ? 0 : number > 1 ? 1 : number) : number;
    }
    return FVM_OK;
}


/* Read a public Array length and its coefficients before touching the owning private state. */
FVM_Result read_array(FVM_Context *c, const FVM_Value &value, FVM_Convolution &data)
{
    data.length = 0;
    for (unsigned i = 0; i < 225; ++i)
    {
        data.matrix[i] = 0;
    }
    if (value.type == FVM_VALUE_NULL || value.type == FVM_VALUE_UNDEFINED)
    {
        return FVM_OK;
    }
    if (value.type != FVM_VALUE_OBJECT || !value.object || value.object->owner != c)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value length;
    FVM_Result r = fvm_object_get(c, value.object, "length", &length);
    double count = 0;
    if (r == FVM_OK)
    {
        r = fvm_value_to_number(c, &length, &count);
    }
    if (r != FVM_OK)
    {
        return r;
    }
    if (!(count >= 0 && count <= 225 && count == std::floor(count)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    data.length = (unsigned)count;
    for (unsigned i = 0; i < data.length; ++i)
    {
        char text[4];
        key(i, text);
        FVM_Value entry;
        r = fvm_object_get(c, value.object, text, &entry);
        if (r != FVM_OK)
        {
            return r;
        }
        if (entry.type == FVM_VALUE_UNDEFINED)
        {
            continue;
        }
        r = fvm_value_to_number(c, &entry, data.matrix + i);
        if (r != FVM_OK)
        {
            return r;
        }
        if (!coefficient_valid(data.matrix[i]))
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    return FVM_OK;
}


/* Extend the coefficient sequence with already initialized zeros; shrinking dimensions preserves
 * it. */
void extend(FVM_Convolution &data)
{
    unsigned count = (unsigned)data.parameters[0] * (unsigned)data.parameters[1];
    if (data.length < count)
    {
        data.length = count;
    }
}


/* [GC:] Publish complete private numeric storage only after every field allocation succeeds. */
FVM_Result store(FVM_Context *c, FVM_Object *self, const FVM_Convolution &data)
{
    FVM_Object *state = 0;
    /* [GC:] Retain unpublished numeric data through the caller's native barrier. */
    FVM_Result r = fvm_object_create(c, &state);
    if (r != FVM_OK)
    {
        return r;
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        state->properties[names[i]] = scalar(data.parameters[i]);
    }
    state->properties["length"] = scalar(data.length);
    for (unsigned i = 0; i < data.length; ++i)
    {
        char text[4];
        key(i, text);
        state->properties[text] = scalar(data.matrix[i]);
    }
    /* [GC:] Replace the private edge atomically after the whole record is populated. */
    self->resource_result = managed(state);
    /* [GC:] Enable tracing and transport of the newly installed managed data. */
    self->resource_result_valid = true;
    return FVM_OK;
}


/* [GC:] Create an independent filter instance using its retained class identity. */
FVM_Result make(FVM_Context *c, void *u, const FVM_Convolution &data, FVM_Value *out)
{
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, u, "ConvolutionFilter", klass);
    if (r == FVM_OK)
    {
        r = create(c, klass, FVM_CONVOLUTION_FILTER, 0, 0, 0, 0, out);
    }
    if (r == FVM_OK)
    {
        r = store(c, out->object, data);
    }
    if (r != FVM_OK)
    {
        fvm_value_undefined(out);
    }
    return r;
}


/* [GC:] Convert constructor arguments before allocating independent convolution storage. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *u)
{


    /* [GC:] Protect converted objects and unpublished filter/class edges throughout construction.
     */
    FVM_NativeCallGuard guard(c);
    FVM_Convolution data;
    FVM_Result r = FVM_OK;
    const unsigned positions[] = {0, 1, 3, 4, 5, 6, 7, 8};
    for (unsigned i = 0; r == FVM_OK && i < 2; ++i)
    {
        if (n > positions[i])
        {
            r = option(c, i, args[positions[i]], data.parameters[i]);
        }
    }
    if (r == FVM_OK && n > 2)
    {
        r = read_array(c, args[2], data);
    }
    for (unsigned i = 2; r == FVM_OK && i < 8; ++i)
    {
        if (n > positions[i])
        {
            r = option(c, i, args[positions[i]], data.parameters[i]);
        }
    }
    extend(data);
    return r == FVM_OK ? make(c, u, data, out) : r;
}


/* [GC:] Clone validated coefficient and option data into a new independently owned filter. */
FVM_Result clone(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{


    /* [GC:] Protect source state and the captured factory while allocating the copy. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_Convolution data;
    return fvm_convolution_values(c, self, data) ? make(c, u, data, out) : FVM_OK;
}


/* [GC:] Return a fresh Array snapshot without exposing private coefficient storage. */
FVM_Result matrix_get(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{


    /* [GC:] Protect source/filter and unpublished Array through all indexed writes. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_Convolution data;
    if (!fvm_convolution_values(c, self, data))
    {
        return FVM_OK;
    }
    FVM_Object *array_template = 0, *array = 0;
    FVM_Result r = captured_class(c, u, "FilterArray", array_template);
    if (r == FVM_OK)
    {
        r = fvm_array_create_like(c, array_template, data.length, &array);
    }
    for (unsigned i = 0; r == FVM_OK && i < data.length; ++i)
    {
        FVM_Value value = scalar(data.matrix[i]);
        r = fvm_table_set_index(c, array, i, &value);
    }
    if (r == FVM_OK)
    {
        *out = managed(array);
    }
    return r;
}


/* [GC:] Convert the assigned Array and refresh options before atomically replacing coefficients. */
FVM_Result matrix_put(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{


    /* [GC:] Retain source and public conversion objects during reentrant property callbacks. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_Convolution previous, replacement;
    if (!n || !fvm_convolution_values(c, self, previous))
    {
        return FVM_OK;
    }
    FVM_Result r = read_array(c, args[0], replacement);
    if (r != FVM_OK)
    {
        return r;
    }
    if (!fvm_convolution_values(c, self, previous))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        replacement.parameters[i] = previous.parameters[i];
    }
    extend(replacement);
    return store(c, self, replacement);
}

/* Read one validated numeric or boolean native option, ignoring invalid borrowed receivers. */
template <unsigned Index>


/* Preserve boolean option types while exposing all other options as numbers. */
FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    FVM_Convolution data;
    if (fvm_convolution_values(c, self, data))
    {
        if (Index == FVM_CONVOLUTION_PRESERVE || Index == FVM_CONVOLUTION_CLAMP)
        {
            fvm_value_boolean(out, data.parameters[Index] != 0);
        }
        else
        {
            fvm_value_number(out, data.parameters[Index]);
        }
    }
    return FVM_OK;
}

/* [GC:] Convert one public assignment, refresh reentrant updates and publish a complete record. */
template <unsigned Index>


/* [GC:] Preserve every option and coefficient if conversion or allocation fails. */
FVM_Result put(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{


    /* [GC:] Protect the receiver and temporary conversion values during public callbacks. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_Convolution data;
    if (!n || !fvm_convolution_values(c, self, data))
    {
        return FVM_OK;
    }
    double converted = 0;
    FVM_Result r = option(c, Index, args[0], converted);
    if (r != FVM_OK)
    {
        return r;
    }
    if (!fvm_convolution_values(c, self, data))
    {
        return FVM_INVALID_ARGUMENT;
    }
    data.parameters[Index] = converted;
    extend(data);
    return store(c, self, data);
}

const FVM_NativeFunction callbacks[] = {constructor, clone, matrix_get, matrix_put};
const FVM_NativeFunction getters[] = {get<0>, get<1>, get<2>, get<3>, get<4>, get<5>, get<6>, get<7>};
const FVM_NativeFunction setters[] = {put<0>, put<1>, put<2>, put<3>, put<4>, put<5>, put<6>, put<7>};
} // namespace


/* Initialize an empty kernel with neutral divisor/bias and transparent edge replacement. */
FVM_Convolution::FVM_Convolution() : length(0)
{
    const double defaults[] = {0, 0, 1, 0, 1, 1, 0, 0};
    for (unsigned i = 0; i < 8; ++i)
    {
        parameters[i] = defaults[i];
    }
    for (unsigned i = 0; i < 225; ++i)
    {
        matrix[i] = 0;
    }
}


/* [GC:] Validate the owned numeric record before returning a detached host copy. */
bool fvm_convolution_values(FVM_Context *c, FVM_Object *self, FVM_Convolution &out)
{
    if (!self || self->owner != c || self->resource_kind != FVM_CONVOLUTION_FILTER || !self->resource_result_valid ||
        self->resource_result.type != FVM_VALUE_OBJECT || !self->resource_result.object ||
        self->resource_result.object->owner != c)
    {
        return false;
    }
    FVM_Object *state = self->resource_result.object;
    Properties::iterator length = state->properties.find("length");
    if (state->resource_kind || length == state->properties.end() || length->second.type != FVM_VALUE_NUMBER ||
        !(length->second.number >= 0 && length->second.number <= 225 &&
          length->second.number == std::floor(length->second.number)))
    {
        return false;
    }
    out.length = (unsigned)length->second.number;
    if (state->properties.size() != 9 + out.length)
    {
        return false;
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        Properties::iterator entry = state->properties.find(names[i]);
        if (entry == state->properties.end() || entry->second.type != FVM_VALUE_NUMBER ||
            (i != FVM_CONVOLUTION_COLOR && !coefficient_valid(entry->second.number)))
        {
            return false;
        }
        out.parameters[i] = entry->second.number;
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(out.parameters[i] >= 0 && out.parameters[i] <= 15 && out.parameters[i] == std::floor(out.parameters[i])))
        {
            return false;
        }
    }
    if (out.length < (unsigned)out.parameters[0] * (unsigned)out.parameters[1] ||
        !(out.parameters[4] == 0 || out.parameters[4] == 1) || !(out.parameters[5] == 0 || out.parameters[5] == 1) ||
        !(out.parameters[6] >= 0 && out.parameters[6] <= 16777215 &&
          out.parameters[6] == std::floor(out.parameters[6])) ||
        !(out.parameters[7] >= 0 && out.parameters[7] <= 1))
    {
        return false;
    }
    for (unsigned i = 0; i < out.length; ++i)
    {
        char text[4];
        key(i, text);
        Properties::iterator entry = state->properties.find(text);
        if (entry == state->properties.end() || entry->second.type != FVM_VALUE_NUMBER ||
            !coefficient_valid(entry->second.number))
        {
            return false;
        }
        out.matrix[i] = entry->second.number;
    }
    return true;
}


/* [GC:] Install the class, inherited base prototype and captured clone/Array factories. */
FVM_Result fvm_convolution_stage(FVM_Context *c,
                                 FVM_Object *space,
                                 FVM_Object *bindings,
                                 FVM_Object *function_proto,
                                 fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{


    /* [GC:] Protect staged prototypes and constructor records before namespace publication. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *base = 0, *klass = 0, *prototype = 0;
    FVM_Result r = captured_class(c, bindings, "BitmapFilter", base);
    FVM_Value parent;
    if (r == FVM_OK)
    {
        r = fvm_object_get(c, base, "prototype", &parent);
    }
    if (r != FVM_OK || parent.type != FVM_VALUE_OBJECT)
    {
        return r != FVM_OK ? r : FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Allocate constructor identity before exposing it to other managed nodes. */
    r = fvm_object_create(c, &klass);
    if (r == FVM_OK)
    { /* [GC:] Allocate its independently managed instance prototype. */
        r = fvm_object_create(c, &prototype);
    }
    if (r != FVM_OK)
    {
        return r;
    }
    klass->resource_kind = FVM_CONVOLUTION_FILTER_CLASS;
    /* [GC:] Constructor identities inherit native Function behavior. */
    klass->prototype = function_proto;
    /* [GC:] Instances inherit the same retained BitmapFilter prototype as other filters. */
    prototype->prototype = parent.object;
    /* [GC:] Retain class/prototype identity in both directions for transported clones. */
    klass->properties["prototype"] = managed(prototype);
    /* [GC:] Preserve reverse constructor identity for instanceof. */
    prototype->properties["constructor"] = managed(klass);
    prototype->attributes["constructor"] = 3;
    FVM_NativeEntry entry;
    entry.callback = constructor;
    /* [GC:] Capture the shared filter, Array and geometry factory graph. */
    entry.root = bindings;
    /* [GC:] Constructor userdata aliases that retained graph. */
    entry.userdata = bindings;
    klass->constructable = klass->invocable = entry;
    constructors["flash.filters.ConvolutionFilter"] = entry;
    entry.callback = clone;
    prototype->methods["clone"] = entry;
    prototype->attributes["clone"] = 3;
    FVM_AccessorEntry accessor;
    /* [GC:] All accessor snapshots retain the same class and Array factory capture. */
    accessor.root = bindings;
    /* [GC:] Accessor userdata shares the traced factory edge. */
    accessor.userdata = bindings;
    accessor.getter = matrix_get;
    accessor.setter = matrix_put;
    prototype->accessors["matrix"] = accessor;
    prototype->attributes["matrix"] = 3;
    for (unsigned i = 0; i < 8; ++i)
    {
        accessor.getter = getters[i];
        accessor.setter = setters[i];
        prototype->accessors[names[i]] = accessor;
        prototype->attributes[names[i]] = 3;
    }
    /* [GC:] Publish the completed class to the staged namespace. */
    space->properties["ConvolutionFilter"] = managed(klass);
    /* [GC:] Native constructors and clones resolve this class through retained bindings. */
    bindings->properties["ConvolutionFilter"] = managed(klass);
    return FVM_OK;
}


/* Validate the constructor edge without running namespace accessors. */
bool fvm_convolution_validate(FVM_Context *c, FVM_Object *space)
{
    if (!space || space->owner != c)
    {
        return false;
    }
    Properties::iterator i = space->properties.find("ConvolutionFilter");
    return i != space->properties.end() && i->second.type == FVM_VALUE_OBJECT && i->second.object &&
           i->second.object->owner == c && i->second.object->resource_kind == FVM_CONVOLUTION_FILTER_CLASS &&
           i->second.object->constructable.callback == constructor;
}


/* Identify native convolution functions using nonoverlapping append-only callback ranges. */
unsigned long fvm_convolution_callback_id(FVM_NativeFunction fn)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        if (fn == callbacks[i])
        {
            return 1610 + i;
        }
    }
    for (unsigned i = 0; i < 8; ++i)
    {
        if (fn == getters[i])
        {
            return 1630 + i;
        }
        if (fn == setters[i])
        {
            return 1640 + i;
        }
    }
    return 0;
}


/* Restore only recognized convolution constructor, clone and property callbacks. */
FVM_NativeFunction fvm_convolution_callback_from_id(unsigned long id)
{
    if (id >= 1610 && id < 1614)
    {
        return callbacks[id - 1610];
    }
    if (id >= 1630 && id < 1638)
    {
        return getters[id - 1630];
    }
    if (id >= 1640 && id < 1648)
    {
        return setters[id - 1640];
    }
    return 0;
}


/* [GC:] Snapshot the complete source so kernel taps outside the requested ROI and self aliases are
 * stable. */
FVM_Result fvm_convolution_apply(
    FVM_Context *c, FVM_Object *self, const FVM_BitmapRegion &region, const FVM_Convolution &data, FVM_Value *out)
{
    FVM_BitmapState *target = fvm_bitmap_pixels(self), *source = fvm_bitmap_pixels(region.source);
    if (!target || !source || target->disposed || source->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    unsigned columns = (unsigned)data.parameters[0], rows = (unsigned)data.parameters[1];
    if (!columns || !rows)
    {
        return FVM_OK;
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!(region.box[i] >= -2147483648.0 && region.box[i] <= 2147483647.0))
        {
            return FVM_OK;
        }
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(region.destination[i] >= -2147483648.0 && region.destination[i] <= 2147483647.0))
        {
            return FVM_OK;
        }
    }
    double sx = std::floor(region.box[0]), sy = std::floor(region.box[1]);
    double dx = std::floor(region.destination[0]), dy = std::floor(region.destination[1]);
    double left = dx < 0 ? -dx : 0, top = dy < 0 ? -dy : 0;
    double right = std::floor(region.box[2]), bottom = std::floor(region.box[3]);
    if (right > target->width - dx)
    {
        right = target->width - dx;
    }
    if (bottom > target->height - dy)
    {
        bottom = target->height - dy;
    }
    if (left >= right || top >= bottom)
    {
        return FVM_OK;
    }
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && source->pixels.size() > limit)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_detail::vector<unsigned char> pixels(source->pixels);
    unsigned long color = (unsigned long)data.parameters[FVM_CONVOLUTION_COLOR];
    unsigned char replacement[] = {(unsigned char)(color >> 16),
                                   (unsigned char)(color >> 8),
                                   (unsigned char)color,
                                   (unsigned char)std::floor(data.parameters[FVM_CONVOLUTION_ALPHA] * 255 + 0.5)};
    double divisor = data.parameters[FVM_CONVOLUTION_DIVISOR];
    if (!divisor)
    {
        divisor = 1;
    }
    bool clamp = data.parameters[FVM_CONVOLUTION_CLAMP] != 0;
    for (double y = top; y < bottom; ++y)
    {
        for (double x = left; x < right; ++x)
        {
            double channels[4] = {0, 0, 0, 0};
            for (unsigned row = 0; row < rows; ++row)
            {
                for (unsigned column = 0; column < columns; ++column)
                {
                    double px = sx + x + column - columns / 2, py = sy + y + row - rows / 2;
                    if (clamp)
                    {
                        px = px < 0 ? 0 : px >= source->width ? source->width - 1 : px;
                        py = py < 0 ? 0 : py >= source->height ? source->height - 1 : py;
                    }
                    const unsigned char *sample = px >= 0 && px < source->width && py >= 0 && py < source->height
                                                      ? pixels.begin() + ((size_t)py * source->width + (unsigned)px) * 4
                                                      : replacement;
                    double weight = data.matrix[row * columns + column];
                    for (unsigned channel = 0; channel < 4; ++channel)
                    {
                        channels[channel] += sample[channel] * weight;
                    }
                }
            }
            unsigned char *dest = target->pixels.begin() + ((size_t)(dy + y) * target->width + (unsigned)(dx + x)) * 4;
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                double value = channels[channel] / divisor + data.parameters[FVM_CONVOLUTION_BIAS];
                dest[channel] = value <= 0 ? 0 : value >= 255 ? 255 : (unsigned char)std::floor(value + 0.5);
            }
            if (data.parameters[FVM_CONVOLUTION_PRESERVE])
            {
                double px = sx + x, py = sy + y;
                if (clamp)
                {
                    px = px < 0 ? 0 : px >= source->width ? source->width - 1 : px;
                    py = py < 0 ? 0 : py >= source->height ? source->height - 1 : py;
                }
                dest[3] = px >= 0 && px < source->width && py >= 0 && py < source->height
                              ? pixels[((size_t)py * source->width + (unsigned)px) * 4 + 3]
                              : replacement[3];
            }
            if (!target->transparent)
            {
                dest[3] = 255;
            }
            if (!dest[3])
            {
                dest[0] = dest[1] = dest[2] = 0;
            }
        }
    }
    return FVM_OK;
}
