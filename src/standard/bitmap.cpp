/* FlashVM: BitmapData pixel storage, exported resource loading, cloning and clipped pixel
 * operations. */
#include "bitmap_state.hpp"
#include "bitmap_draw.hpp"
#include "bitmap_ops.hpp"
#include "bitmap_perlin.hpp"
#include "bitmap_dissolve.hpp"
#include "filter_state.hpp"
#include "color_transform_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <cstring>

namespace
{
using namespace fvm_geometry_detail;
const unsigned int bitmap_kind = 52;
const unsigned int bitmap_class_kind = 53;
/* [GC:] Delete pixel storage regardless of whether a script finalizer is installed. */
void release(void *pointer)
{
    delete (FVM_BitmapState *)pointer;
}
/* [GC:] Resolve only bitmap-owned storage with the expected internal resource destructor. */
FVM_BitmapState *state(FVM_Object *o)
{
    return o && o->resource_kind == bitmap_kind && o->release_resource == release
               ? (FVM_BitmapState *)o->resource
               : 0;
}
/* Convert finite numeric input to a wrapping 32-bit pixel value. */
unsigned long word(double n)
{
    if (n != n || n > 1.7976931348623157e308 || n < -1.7976931348623157e308)
        return 0;
    n = std::fmod(n < 0 ? std::ceil(n) : std::floor(n), 4294967296.0);
    return (unsigned long)(n < 0 ? n + 4294967296.0 : n);
}
/* Convert script truthiness for the transparent constructor flag. */
bool truth(const FVM_Value &v)
{
    if (v.type == FVM_VALUE_BOOLEAN)
        return v.boolean != 0;
    if (v.type == FVM_VALUE_NUMBER)
        return v.number != 0 && v.number == v.number;
    if (v.type == FVM_VALUE_STRING)
        return fvm_value_string_size(&v) != 0;
    return v.type == FVM_VALUE_OBJECT && v.object;
}
/* Store ARGB as native RGBA, enforcing opaque alpha when transparency is disabled. */
void pixel(FVM_BitmapState *s, size_t index, unsigned long argb)
{
    unsigned char *p = s->pixels.begin() + index * 4;
    p[0] = (unsigned char)(argb >> 16);
    p[1] = (unsigned char)(argb >> 8);
    p[2] = (unsigned char)argb;
    p[3] = s->transparent ? (unsigned char)(argb >> 24) : 255;
}
/* [GC:] Publish an independently owned pixel buffer only after the typed instance exists. */
FVM_Result attach(FVM_Context *c, FVM_Object *klass, FVM_BitmapState *s, FVM_Value *out)
{
    FVM_Result r = create(c, klass, bitmap_kind, 0, 0, 0, 0, out);
    if (r != FVM_OK)
    {
        delete s;
        return r;
    }
    /* [GC:] Transfer native allocation ownership to the managed instance. */
    out->object->resource = s;
    /* [GC:] The collector/context destructor releases pixels even after user-finalizer replacement.
     */
    out->object->release_resource = release;
    return FVM_OK;
}
/* Construct a bounded bitmap using the documented transparent/fill defaults. */
FVM_Result constructor(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args,
                       FVM_Value *out, void *u)
{
    double width = 0, height = 0, fill = 4294967295.0;
    FVM_Result r = numeric_argument(c, n, args, 0, width);
    if (r == FVM_OK)
        r = numeric_argument(c, n, args, 1, height);
    if (r == FVM_OK && n > 3)
        r = numeric_argument(c, n, args, 3, fill);
    if (r != FVM_OK)
        return r;
    /* [GC:] Keep unpublished instance/pixel allocations protected during construction. */
    FVM_NativeCallGuard guard(c);
    FVM_BitmapState *s = new FVM_BitmapState;
    try
    {
        if (width >= 1 && width < 2881 && height >= 1 && height < 2881)
        {
            s->width = (unsigned int)width;
            s->height = (unsigned int)height;
            s->transparent = n > 2 ? truth(args[2]) : true;
            s->disposed = false;
            s->pixels.resize((size_t)s->width * s->height * 4);
            for (size_t i = 0; i < (size_t)s->width * s->height; ++i)
                pixel(s, i, word(fill));
        }
    }
    catch (...)
    {
        delete s;
        throw;
    }
    return attach(c, (FVM_Object *)u, s, out);
}
/* [GC:] Borrow a private owned graph edge without invoking script getters or retaining pointers. */
FVM_Object *owned_edge(FVM_Context *c, FVM_Object *owner, const char *name)
{
    if (!owner || owner->owner != c)
        return 0;
    Properties::iterator found = owner->properties.find(name);
    return found != owner->properties.end() && found->second.type == FVM_VALUE_OBJECT &&
                   found->second.object && found->second.object->owner == c
               ? found->second.object
               : 0;
}

/* [GC:] Find the nearest retained AVM1 MovieClip receiver, falling back to host-bound _root. */
FVM_Result resource_clip(FVM_Context *c, FVM_Object *&clip)
{
    clip = 0;
    /* [GC:] Active AVM1 frames retain receivers while this native callback borrows their identity.
     */
    for (FVM_GCRootFrame *frame = c->gc_frames; frame; frame = frame->previous)
        /* [GC:] The selected receiver already belongs to the active traced call chain. */
        if (frame->receiver && frame->receiver->owner == c && frame->receiver->resource_kind == FVM_RESOURCE_MOVIECLIP)
        {
            /* [GC:] Borrow the current clip only while the enclosing native barrier remains active.
             */
            clip = frame->receiver;
            return FVM_OK;
        }
    /* [GC:] Hold the global root value under the active native collector barrier. */
    FVM_Value root;
    /* [GC:] Global bindings retain a host-selected fallback movie resource scope. */
    FVM_Result r = fvm_get_global(c, "_root", &root);
    if (r != FVM_OK)
        return r;
    /* [GC:] Select only a context-owned MovieClip retained through the global root edge. */
    if (root.type == FVM_VALUE_OBJECT && root.object && root.object->owner == c &&
        /* [GC:] Reject non-clip objects as resource scopes. */
        root.object->resource_kind == FVM_RESOURCE_MOVIECLIP)
        /* [GC:] Borrow its identity until native callback completion. */
        clip = root.object;
    return FVM_OK;
}

/* [GC:] Copy one exported dictionary bitmap through the calling class's retained prototype. */
FVM_Result load_bitmap(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args,
                       FVM_Value *out, void *u)
{
    /* [GC:] Keep the caller's resource graph and imported factory class alive during coercion/copy.
     */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    out->type = FVM_VALUE_NULL;
    FVM_Value name, input = argument(n, args, 0);
    FVM_Result r = fvm_value_to_string(c, &input, &name);
    if (r != FVM_OK)
        return r;
    FVM_Object *clip = 0;
    r = resource_clip(c, clip);
    if (r != FVM_OK || !clip)
        return r;
    if (!clip->resource_result_valid || clip->resource_result.type != FVM_VALUE_OBJECT)
        return FVM_OK;
    /* [GC:] Traverse only dictionary edges retained by the selected live clip. */
    FVM_Object *library = owned_edge(c, clip->resource_result.object, "library");
    /* [GC:] Linkage exports retain their resource definitions. */
    FVM_Object *exports = owned_edge(c, library, "exports");
    if (!exports)
        return FVM_OK;
    fvm_detail::string key(name.string, fvm_value_string_size(&name));
    Properties::iterator exported = exports->properties.find(key);
    if (exported == exports->properties.end() || exported->second.type != FVM_VALUE_OBJECT)
        return FVM_OK;
    /* [GC:] The resource definition retains its original native bitmap pixels. */
    FVM_Object *bitmap = owned_edge(c, exported->second.object, "bitmap_data");
    FVM_BitmapState *source = state(bitmap);
    if (!source || source->disposed)
        return FVM_OK;
    /* [GC:] Allocate independently owned pixels; mutating the result never changes dictionary art.
     */
    FVM_BitmapState *copy = new FVM_BitmapState;
    try
    {
        *copy = *source;
    }
    catch (...)
    {
        delete copy;
        throw;
    }
    return attach(c, (FVM_Object *)u, copy, out);
}

/* Map public BitmapDataChannel bit flags to native RGBA byte offsets. */
int channel_index(unsigned long channel)
{
    switch (channel)
    {
    case 1:
        return 0;
    case 2:
        return 1;
    case 4:
        return 2;
    case 8:
        return 3;
    default:
        return -1;
    }
}
/* Return immutable size/transparency properties or a typed independent Rectangle. */
template <unsigned Property>
/* Read one BitmapData property, returning -1 for disposed/native-invalid receivers. */
FVM_Result get(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out,
               void *u)
{
    FVM_BitmapState *s = state(self);
    if (!s || s->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    if (Property < 2)
    {
        fvm_value_number(out, Property ? s->height : s->width);
        return FVM_OK;
    }
    if (Property == 2)
    {
        fvm_value_boolean(out, s->transparent);
        return FVM_OK;
    }
    FVM_Object *klass = 0;
    FVM_Result r = captured_class(c, u, "Rectangle", klass);
    const char *names[] = {"x", "y", "width", "height"};
    FVM_Value values[] = {scalar(0), scalar(0), scalar(s->width), scalar(s->height)};
    return r == FVM_OK ? create(c, klass, FVM_GEOMETRY_RECTANGLE, names, 4, values, 0, out) : r;
}
/* Read/write pixels after argument coercion, preserving alpha for setPixel. */
template <unsigned Operation>
/* Execute clone/dispose/getPixel/getPixel32/setPixel/setPixel32/fillRect without exposing pointers.
 */
FVM_Result method(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args,
                  FVM_Value *out, void *u)
{
    fvm_value_undefined(out);
    FVM_BitmapState *s = state(self);
    if (!s || s->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    if (Operation == 0)
    {
        FVM_Object *klass = 0;
        FVM_Result r = captured_class(c, u, "BitmapData", klass);
        if (r != FVM_OK)
            return r;
        /* [GC:] Clone bytes into a separate native allocation; no pointer crosses the factory
         * boundary. */
        FVM_BitmapState *copy = new FVM_BitmapState;
        try
        {
            *copy = *s;
        }
        catch (...)
        {
            delete copy;
            throw;
        }
        return attach(c, klass, copy, out);
    }
    if (Operation == 1)
    {
        fvm_detail::vector<unsigned char> empty;
        s->pixels.swap(empty);
        s->width = s->height = 0;
        s->disposed = true;
        return FVM_OK;
    }
    if (Operation == 9)
    {
        FVM_Object *source = 0, *rect = 0, *point = 0;
        FVM_Result r = object_argument(c, n, args, 0, source);
        if (r == FVM_OK)
            r = object_argument(c, n, args, 1, rect);
        const char *rect_names[] = {"x", "y", "width", "height"};
        const char *point_names[] = {"x", "y"};
        double box[4], destination[2], from = 0, to = 0;
        if (r == FVM_OK)
            r = numbers(c, rect, rect_names, 4, box);
        if (r == FVM_OK)
            r = object_argument(c, n, args, 2, point);
        if (r == FVM_OK)
            r = numbers(c, point, point_names, 2, destination);
        if (r == FVM_OK)
            r = numeric_argument(c, n, args, 3, from);
        if (r == FVM_OK)
            r = numeric_argument(c, n, args, 4, to);
        if (r != FVM_OK)
            return r;
        FVM_BitmapState *input = state(source);
        if (!input || input->disposed || s->disposed)
        {
            fvm_value_number(out, -1);
            return FVM_OK;
        }
        int source_channel = channel_index(word(from)), dest_channel = channel_index(word(to));
        if (source_channel < 0 || dest_channel < 0 || (!s->transparent && dest_channel == 3))
            return FVM_OK;
        /* Bound coordinates before integer conversion and clip in source-relative space. */
        for (unsigned i = 0; i < 4; ++i)
            if (!(box[i] >= -2147483648.0 && box[i] <= 2147483647.0))
                return FVM_OK;
        for (unsigned i = 0; i < 2; ++i)
            if (!(destination[i] >= -2147483648.0 && destination[i] <= 2147483647.0))
                return FVM_OK;
        double sx = std::floor(box[0]), sy = std::floor(box[1]);
        double dx = std::floor(destination[0]), dy = std::floor(destination[1]);
        double left = 0, top = 0, right = std::floor(box[2]), bottom = std::floor(box[3]);
        if (left < -sx)
            left = -sx;
        if (left < -dx)
            left = -dx;
        if (top < -sy)
            top = -sy;
        if (top < -dy)
            top = -dy;
        if (right > input->width - sx)
            right = input->width - sx;
        if (right > s->width - dx)
            right = s->width - dx;
        if (bottom > input->height - sy)
            bottom = input->height - sy;
        if (bottom > s->height - dy)
            bottom = s->height - dy;
        if (!(left < right && top < bottom))
            return FVM_OK;
        /* Snapshot only the selected channel. This also makes overlapping self-copy atomic on OOM.
         */
        unsigned width = (unsigned)(right - left), height = (unsigned)(bottom - top);
        fvm_detail::vector<unsigned char> selected((size_t)width * height);
        for (unsigned y = 0; y < height; ++y)
            for (unsigned x = 0; x < width; ++x)
                selected[(size_t)y * width + x] =
                    input
                        ->pixels[((size_t)(sy + top + y) * input->width + (size_t)(sx + left + x)) *
                                     4 +
                                 source_channel];
        for (unsigned y = 0; y < height; ++y)
            for (unsigned x = 0; x < width; ++x)
                s->pixels[((size_t)(dy + top + y) * s->width + (size_t)(dx + left + x)) * 4 +
                          dest_channel] = selected[(size_t)y * width + x];
        return FVM_OK;
    }
    if (Operation == 8)
    {
        FVM_Object *rect = 0, *transform = 0;
        FVM_Result r = object_argument(c, n, args, 0, rect);
        const char *names[] = {"x", "y", "width", "height"};
        double box[4], channels[8];
        if (r == FVM_OK)
            r = numbers(c, rect, names, 4, box);
        if (r == FVM_OK)
            r = object_argument(c, n, args, 1, transform);
        if (r != FVM_OK)
            return r;
        if (!fvm_color_transform_values(c, transform, channels))
            return FVM_INVALID_ARGUMENT;
        if (s->disposed)
        {
            fvm_value_number(out, -1);
            return FVM_OK;
        }
        if (!(box[2] > 0 && box[3] > 0))
            return FVM_OK;
        double left = std::floor(box[0]), top = std::floor(box[1]);
        double right = std::floor(box[0] + box[2]), bottom = std::floor(box[1] + box[3]);
        if (left < 0)
            left = 0;
        if (top < 0)
            top = 0;
        if (right > s->width)
            right = s->width;
        if (bottom > s->height)
            bottom = s->height;
        if (!(left < right && top < bottom))
            return FVM_OK;
        for (unsigned y = (unsigned)top; y < (unsigned)bottom; ++y)
            for (unsigned x = (unsigned)left; x < (unsigned)right; ++x)
            {
                unsigned char *p = s->pixels.begin() + ((size_t)y * s->width + x) * 4;
                for (unsigned channel = 0; channel < 4; ++channel)
                {
                    double value = p[channel] * channels[channel] + channels[channel + 4];
                    p[channel] = !(value > 0) ? 0 : value >= 255 ? 255 : (unsigned char)value;
                }
                if (!s->transparent)
                    p[3] = 255;
            }
        return FVM_OK;
    }
    if (Operation == 7)
    {
        double dx = 0, dy = 0;
        FVM_Result r = numeric_argument(c, n, args, 0, dx);
        if (r == FVM_OK)
            r = numeric_argument(c, n, args, 1, dy);
        if (r != FVM_OK)
            return r;
        if (s->disposed)
        {
            fvm_value_number(out, -1);
            return FVM_OK;
        }
        dx = dx < 0 ? std::ceil(dx) : std::floor(dx);
        dy = dy < 0 ? std::ceil(dy) : std::floor(dy);
        if (!(dx > -(double)s->width && dx < s->width && dy > -(double)s->height && dy < s->height))
            return FVM_OK;
        int shift_x = (int)dx, shift_y = (int)dy;
        /* Read the original image throughout; overlap must not smear adjacent pixels.
         * Allocate before mutation so allocation failure leaves the bitmap unchanged. */
        fvm_detail::vector<unsigned char> original(s->pixels);
        for (unsigned y = 0; y < s->height; ++y)
            for (unsigned x = 0; x < s->width; ++x)
            {
                int source_x = (int)x - shift_x, source_y = (int)y - shift_y;
                if (source_x >= 0 && source_x < (int)s->width && source_y >= 0 &&
                    source_y < (int)s->height)
                    std::memcpy(s->pixels.begin() + ((size_t)y * s->width + x) * 4,
                                original.begin() + ((size_t)source_y * s->width + source_x) * 4, 4);
            }
        return FVM_OK;
    }
    if (Operation == 6)
    {
        FVM_Object *rect = 0;
        FVM_Result r = object_argument(c, n, args, 0, rect);
        const char *names[] = {"x", "y", "width", "height"};
        double box[4], color = 0;
        if (r == FVM_OK)
            r = numbers(c, rect, names, 4, box);
        if (r == FVM_OK)
            r = numeric_argument(c, n, args, 1, color);
        if (r != FVM_OK)
            return r;
        if (s->disposed)
        {
            fvm_value_number(out, -1);
            return FVM_OK;
        }
        if (!(box[2] > 0 && box[3] > 0))
            return FVM_OK;
        double left = std::floor(box[0]), top = std::floor(box[1]);
        double right = std::floor(box[0] + box[2]), bottom = std::floor(box[1] + box[3]);
        if (left < 0)
            left = 0;
        if (top < 0)
            top = 0;
        if (right > s->width)
            right = s->width;
        if (bottom > s->height)
            bottom = s->height;
        if (!(left < right && top < bottom))
            return FVM_OK;
        for (unsigned y = (unsigned)top; y < (unsigned)bottom; ++y)
            for (unsigned x = (unsigned)left; x < (unsigned)right; ++x)
                pixel(s, (size_t)y * s->width + x, word(color));
        return FVM_OK;
    }
    double x = 0, y = 0, color = 0;
    FVM_Result r = numeric_argument(c, n, args, 0, x);
    if (r == FVM_OK)
        r = numeric_argument(c, n, args, 1, y);
    if (r == FVM_OK && Operation >= 4)
        r = numeric_argument(c, n, args, 2, color);
    if (r != FVM_OK)
        return r;
    if (s->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    x = x < 0 ? std::ceil(x) : std::floor(x);
    y = y < 0 ? std::ceil(y) : std::floor(y);
    if (!(x >= 0 && x < s->width && y >= 0 && y < s->height))
    {
        if (Operation < 4)
            fvm_value_number(out, 0);
        return FVM_OK;
    }
    size_t index = (size_t)y * s->width + (size_t)x;
    unsigned char *p = s->pixels.begin() + index * 4;
    if (Operation < 4)
    {
        unsigned long rgb = ((unsigned long)p[0] << 16) | ((unsigned long)p[1] << 8) | p[2];
        unsigned long value = Operation == 3 ? rgb | ((unsigned long)p[3] << 24) : rgb;
        /* AVM1 exposes getPixel32 as signed Int32 even though input colors wrap as Uint32. */
        fvm_value_number(out, value >= 2147483648UL ? (double)value - 4294967296.0 : (double)value);
    }
    else
    {
        unsigned long argb = word(color);
        if (Operation == 4)
            argb = (argb & 0xffffffUL) | ((unsigned long)p[3] << 24);
        pixel(s, index, argb);
    }
    return FVM_OK;
}
const char *const properties[] = {"width", "height", "transparent", "rectangle"};
const FVM_NativeFunction getters[] = {get<0>, get<1>, get<2>, get<3>};
const char *const methods[] = {
    "clone",      "dispose",     "getPixel",     "getPixel32",         "setPixel",
    "setPixel32", "fillRect",    "scroll",       "colorTransform",     "copyChannel",
    "copyPixels", "draw",        "floodFill",    "getColorBoundsRect", "threshold",
    "merge",      "paletteMap",  "hitTest",      "applyFilter",        "generateFilterRect",
    "noise",      "perlinNoise", "pixelDissolve"};
const FVM_NativeFunction callbacks[] = {method<0>,
                                        method<1>,
                                        method<2>,
                                        method<3>,
                                        method<4>,
                                        method<5>,
                                        method<6>,
                                        method<7>,
                                        method<8>,
                                        method<9>,
                                        fvm_bitmap_copy_pixels,
                                        fvm_bitmap_draw,
                                        fvm_bitmap_flood_fill,
                                        fvm_bitmap_color_bounds,
                                        fvm_bitmap_threshold,
                                        fvm_bitmap_merge,
                                        fvm_bitmap_palette_map,
                                        fvm_bitmap_hit_test,
                                        fvm_bitmap_apply_filter,
                                        fvm_bitmap_filter_rect,
                                        fvm_bitmap_noise,
                                        fvm_bitmap_perlin_noise,
                                        fvm_bitmap_pixel_dissolve};
} // namespace
/* [GC:] Borrow the native pixel allocation only when its internal ownership callback matches. */
FVM_BitmapState *fvm_bitmap_pixels(FVM_Object *object)
{
    return state(object);
}
/* [GC:] Build a resource bitmap with independent pixels and optional installed class identity. */
FVM_Result fvm_bitmap_create_pixels(FVM_Context *c, unsigned width, unsigned height,
                                    bool transparent,
                                    const fvm_detail::vector<unsigned char> &pixels, FVM_Value *out)
{
    if (!c || !out || !width || !height || width > 2880 || height > 2880 ||
        pixels.size() != (size_t)width * height * 4)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    /* [GC:] Protect the unpublished bitmap and any borrowed class/prototype factory. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *klass = 0;
    fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator registered =
        c->constructors.find("flash.display.BitmapData");
    if (registered != c->constructors.end() && registered->second.callback == constructor)
        /* [GC:] The trusted constructor capture retains the original native bitmap class. */
        klass = registered->second.root;
    FVM_BitmapState *storage = new FVM_BitmapState;
    try
    {
        storage->width = width;
        storage->height = height;
        storage->transparent = transparent;
        storage->disposed = false;
        storage->pixels = pixels;
    }
    catch (...)
    {
        delete storage;
        throw;
    }
    if (klass)
        return attach(c, klass, storage, out);
    /* [GC:] Basic contexts can render a private bitmap even without installed script classes. */
    FVM_Object *object = 0;
    FVM_Result result = fvm_object_create(c, &object);
    if (result != FVM_OK)
    {
        delete storage;
        return result;
    }
    object->resource_kind = bitmap_kind;
    /* [GC:] Transfer native pixel ownership only after the managed resource exists. */
    object->resource = storage;
    /* [GC:] Native pixel cleanup remains independent of script finalizers. */
    object->release_resource = release;
    /* [GC:] Return the completed managed edge for resource-dictionary adoption. */
    *out = managed(object);
    return FVM_OK;
}
/* [GC:] Install an unpublished display namespace and bitmap class with retained rectangle/clone
 * factories. */
FVM_Result fvm_bitmap_stage(FVM_Context *c, FVM_Object *flash, FVM_Object *bindings,
                            FVM_Object *object_proto, FVM_Object *function_proto,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &constructors)
{
    /* [GC:] Protect every staged namespace/class edge until the installer publishes globals. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *display = 0, *klass = 0, *prototype = 0;
    /* [GC:] Allocate the display namespace. */
    FVM_Result r = fvm_object_create(c, &display);
    if (r == FVM_OK)
        r = fvm_object_create(c, &klass);
    if (r == FVM_OK)
        r = fvm_object_create(c, &prototype);
    if (r != FVM_OK)
        return r;
    klass->resource_kind = bitmap_class_kind;
    /* [GC:] Preserve namespace/Object/Function inheritance and constructor cycles. */
    display->prototype = object_proto;
    /* [GC:] Retain Function inheritance for constructors. */
    klass->prototype = function_proto;
    /* [GC:] Retain Object inheritance for instances. */
    prototype->prototype = object_proto;
    /* [GC:] Class and prototype retain each other for transported identity. */
    klass->properties["prototype"] = managed(prototype);
    /* [GC:] Instances retain constructor identity through their prototype. */
    prototype->properties["constructor"] = managed(klass);
    prototype->attributes["constructor"] = 3;
    FVM_NativeEntry entry;
    entry.callback = constructor;
    /* [GC:] Constructor userdata aliases its traced class root. */
    entry.userdata = klass;
    /* [GC:] Keep constructor class/prototype alive through native entries. */
    entry.root = klass;
    klass->constructable = klass->invocable = entry;
    entry.callback = load_bitmap;
    /* [GC:] Static loads retain this class/prototype, including after graph transport. */
    klass->methods["loadBitmap"] = entry;
    klass->attributes["loadBitmap"] = 3;
    entry.callback = constructor;
    constructors["flash.display.BitmapData"] = entry;
    /* [GC:] Namespace and factory captures retain the bitmap class. */
    display->properties["BitmapData"] = managed(klass);
    /* [GC:] Factories resolve BitmapData through the same retained geometry binding graph. */
    bindings->properties["BitmapData"] = managed(klass);
    /* [GC:] The staged flash namespace owns the display namespace. */
    flash->properties["display"] = managed(display);
    for (unsigned i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i)
    {
        entry.callback = callbacks[i];
        /* [GC:] Method userdata aliases retained class factories. */
        entry.userdata = bindings;
        /* [GC:] Clone callbacks preserve their factory classes across transport. */
        entry.root = bindings;
        prototype->methods[methods[i]] = entry;
        prototype->attributes[methods[i]] = 3;
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        FVM_AccessorEntry a;
        a.getter = getters[i];
        /* [GC:] Rectangle getter userdata aliases its retained factory graph. */
        a.userdata = bindings;
        /* [GC:] Preserve Rectangle/BitmapData identity through accessor captures. */
        a.root = bindings;
        prototype->accessors[properties[i]] = a;
        prototype->attributes[properties[i]] = 7;
    }
    return FVM_OK;
}
/* Validate the display namespace and bitmap native constructor without invoking user code. */
bool fvm_bitmap_validate(FVM_Context *c, FVM_Object *flash)
{
    Properties::iterator d = flash->properties.find("display");
    if (d == flash->properties.end() || d->second.type != FVM_VALUE_OBJECT || !d->second.object ||
        d->second.object->owner != c)
        return false;
    Properties::iterator k = d->second.object->properties.find("BitmapData");
    return k != d->second.object->properties.end() && k->second.type == FVM_VALUE_OBJECT &&
           k->second.object && k->second.object->owner == c &&
           k->second.object->resource_kind == bitmap_class_kind &&
           k->second.object->constructable.callback == constructor;
}
/* Identify bitmap constructor/method/accessor implementations with stable IDs. */
unsigned long fvm_bitmap_callback_id(FVM_NativeFunction fn)
{
    if (fn == constructor)
        return 1500;
    if (fn == load_bitmap)
        return 1501;
    for (unsigned i = 0; i < sizeof(callbacks) / sizeof(callbacks[0]); ++i)
        if (fn == callbacks[i])
            return i < 11 ? 1510 + i : 1540 + i - 11;
    for (unsigned i = 0; i < 4; ++i)
        if (fn == getters[i])
            return 1530 + i;
    return 0;
}
/* Restore only known bitmap native implementation IDs. */
FVM_NativeFunction fvm_bitmap_callback_from_id(unsigned long id)
{
    if (id == 1500)
        return constructor;
    if (id == 1501)
        return load_bitmap;
    if (id >= 1510 && id < 1521)
        return callbacks[id - 1510];
    if (id >= 1540 && id < 1540 + sizeof(callbacks) / sizeof(callbacks[0]) - 11)
        return callbacks[id - 1540 + 11];
    if (id >= 1530 && id < 1534)
        return getters[id - 1530];
    return 0;
}
/* Snapshot dimensions/flags and arbitrary pixel bytes without encoding them as script text. */
FVM_Result fvm_bitmap_snapshot(FVM_Object *o, fvm_detail::string &out)
{
    FVM_BitmapState *s = state(o);
    if (!s)
        return FVM_UNSUPPORTED;
    fvm_detail::vector<char> bytes(12 + s->pixels.size());
    std::memcpy(bytes.begin(), "FBM1", 4);
    bytes[4] = (char)s->width;
    bytes[5] = (char)(s->width >> 8);
    bytes[6] = (char)s->height;
    bytes[7] = (char)(s->height >> 8);
    bytes[8] = s->transparent ? 1 : 0;
    bytes[9] = s->disposed ? 1 : 0;
    bytes[10] = bytes[11] = 0;
    if (s->pixels.size())
        std::memcpy(bytes.begin() + 12, s->pixels.begin(), s->pixels.size());
    out = fvm_detail::string(bytes.begin(), bytes.size());
    return FVM_OK;
}
/* Validate exact dimensions, flags and canonical pixel length before restoring allocations. */
bool fvm_bitmap_snapshot_valid(const fvm_detail::string &bytes)
{
    if (bytes.size() < 12 || std::memcmp(bytes.c_str(), "FBM1", 4) || (unsigned char)bytes[8] > 1 ||
        (unsigned char)bytes[9] > 1 || bytes[10] || bytes[11])
        return false;
    unsigned w = (unsigned char)bytes[4] | ((unsigned)(unsigned char)bytes[5] << 8),
             h = (unsigned char)bytes[6] | ((unsigned)(unsigned char)bytes[7] << 8);
    if (bytes[9])
        return !w && !h && bytes.size() == 12;
    if (!(w >= 1 && w <= 2880 && h >= 1 && h <= 2880 && bytes.size() == 12 + (size_t)w * h * 4))
        return false;
    if (!bytes[8])
        for (size_t i = 15; i < bytes.size(); i += 4)
            if ((unsigned char)bytes[i] != 255)
                return false;
    return true;
}
/* [GC:] Restore independent pixel storage and attach its internal destructor only after allocation
 * succeeds. */
FVM_Result fvm_bitmap_restore(FVM_Object *o, const fvm_detail::string &bytes)
{
    if (!o || !fvm_bitmap_snapshot_valid(bytes))
        return FVM_INVALID_ARGUMENT;
    FVM_BitmapState *s = new FVM_BitmapState;
    try
    {
        s->width = (unsigned char)bytes[4] | ((unsigned)(unsigned char)bytes[5] << 8);
        s->height = (unsigned char)bytes[6] | ((unsigned)(unsigned char)bytes[7] << 8);
        s->transparent = bytes[8] != 0;
        s->disposed = bytes[9] != 0;
        s->pixels.resize(bytes.size() - 12);
        if (s->pixels.size())
            std::memcpy(s->pixels.begin(), bytes.c_str() + 12, s->pixels.size());
    }
    catch (...)
    {
        delete s;
        throw;
    }
    /* [GC:] Transfer the completed native allocation to this restored managed instance. */
    o->resource = s;
    /* [GC:] Internal cleanup stays independent of user finalizers. */
    o->release_resource = release;
    return FVM_OK;
}
