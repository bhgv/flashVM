/* FlashVM: bounded aligned source/destination clipping and copied RGBA regions for bitmap APIs. */
#include "bitmap_region.hpp"
#include <cmath>
#include <cstring>


/* Initialize an empty temporary view without creating managed objects or pixel allocations. */
FVM_BitmapRegion::FVM_BitmapRegion() : source(0), sx(0), sy(0), dx(0), dy(0), width(0), height(0)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        box[i] = 0;
    }
    destination[0] = destination[1] = 0;
}


/* [GC:] Read public tuples while caller-protected source and argument objects may run user getters.
 */
FVM_Result fvm_bitmap_region_read(FVM_Context *c, unsigned n, const FVM_Value *args, FVM_BitmapRegion &region)
{
    using namespace fvm_geometry_detail;
    FVM_Object *rect = 0, *point = 0;
    const char *const rect_names[] = {"x", "y", "width", "height"};
    const char *const point_names[] = {"x", "y"};
    FVM_Result r = object_argument(c, n, args, 0, region.source);
    if (r == FVM_OK)
    {
        r = object_argument(c, n, args, 1, rect);
    }
    if (r == FVM_OK)
    {
        r = numbers(c, rect, rect_names, 4, region.box);
    }
    if (r == FVM_OK)
    {
        r = object_argument(c, n, args, 2, point);
    }
    if (r == FVM_OK)
    {
        r = numbers(c, point, point_names, 2, region.destination);
    }
    return r;
}


/* [GC:] Revalidate native storage after callbacks, then clip copied integer coordinates safely. */
FVM_Result
fvm_bitmap_region_clip(FVM_Object *self, FVM_BitmapRegion &region, FVM_BitmapState *&target, FVM_BitmapState *&input)
{
    target = fvm_bitmap_pixels(self);
    input = fvm_bitmap_pixels(region.source);
    if (!target || !input || target->disposed || input->disposed)
    {
        return FVM_INVALID_ARGUMENT;
    }
    region.width = region.height = 0;
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
    double left = 0, top = 0, right = std::floor(region.box[2]), bottom = std::floor(region.box[3]);
    if (left < -sx)
    {
        left = -sx;
    }
    if (left < -dx)
    {
        left = -dx;
    }
    if (top < -sy)
    {
        top = -sy;
    }
    if (top < -dy)
    {
        top = -dy;
    }
    if (right > input->width - sx)
    {
        right = input->width - sx;
    }
    if (right > target->width - dx)
    {
        right = target->width - dx;
    }
    if (bottom > input->height - sy)
    {
        bottom = input->height - sy;
    }
    if (bottom > target->height - dy)
    {
        bottom = target->height - dy;
    }
    if (left < right && top < bottom)
    {
        region.sx = (unsigned)(sx + left);
        region.sy = (unsigned)(sy + top);
        region.dx = (unsigned)(dx + left);
        region.dy = (unsigned)(dy + top);
        region.width = (unsigned)(right - left);
        region.height = (unsigned)(bottom - top);
    }
    return FVM_OK;
}


/* Copy eligible rows only, checking scratch bytes before any destination pixel can be edited. */
FVM_Result fvm_bitmap_region_snapshot(FVM_Context *c,
                                      const FVM_BitmapRegion &region,
                                      const FVM_BitmapState *source,
                                      fvm_detail::vector<unsigned char> &pixels)
{
    size_t bytes = (size_t)region.width * region.height * 4;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && bytes > limit)
    {
        return FVM_OUT_OF_MEMORY;
    }
    fvm_detail::vector<unsigned char> snapshot(bytes);
    for (unsigned y = 0; y < region.height; ++y)
    {
        std::memcpy(snapshot.begin() + (size_t)y * region.width * 4,
                    source->pixels.begin() + ((size_t)(region.sy + y) * source->width + region.sx) * 4,
                    (size_t)region.width * 4);
    }
    pixels.swap(snapshot);
    return FVM_OK;
}
