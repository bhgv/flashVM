/* Checked RGBA row copies: validate both complete visible extents before touching destination. */
#include "frame_internal.h"
#include <string.h>


/* Initialize native callback policy without allocating or entering any subsystem. */
int fvm_desktop_frame_observer_init(FVM_DesktopFrameObserver *out)
{
    if (!out)
    {
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    return 1;
}


/* A copied observer descriptor may not silently discard every rendered frame. */
int fvm_desktop_frame_observer_valid(const FVM_DesktopFrameObserver *observer)
{
    return observer && observer->struct_size == sizeof(*observer) && observer->notify;
}


/* Avoid overflow in (height-1)*stride+row_bytes, even with a deliberately huge native stride. */
static int frame_extent(size_t row_bytes, unsigned int height, size_t stride, size_t capacity)
{
    size_t rows = (size_t)(height - 1);
    if (stride < row_bytes || rows > ((size_t)-1 - row_bytes) / stride)
    {
        return 0;
    }
    return rows * stride + row_bytes <= capacity;
}


/* No copy can begin until format, dimensions, strides and both storage spans have passed. */
FVM_Result
fvm_desktop_frame_copy(const FVM_DesktopFrame *frame, void *destination, size_t capacity, size_t destination_stride)
{
    size_t row_bytes, source_stride;
    unsigned int y, height;
    const unsigned char *pixels;
    unsigned char *target = (unsigned char *)destination;
    if (!frame || frame->struct_size != sizeof(*frame))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (frame->format != FVM_DESKTOP_FRAME_RGBA8 || !frame->pixels || !destination || !frame->width || !frame->height ||
        frame->width > FVM_DESKTOP_FRAME_MAX_DIMENSION || frame->height > FVM_DESKTOP_FRAME_MAX_DIMENSION)
    {
        return FVM_INVALID_ARGUMENT;
    }
    row_bytes = (size_t)frame->width * FVM_DESKTOP_FRAME_PIXEL_BYTES;
    /* Cache only copy geometry, not unused metadata, before output may overwrite the descriptor. */
    source_stride = frame->stride;
    height = frame->height;
    pixels = frame->pixels;
    if (!frame_extent(row_bytes, height, source_stride, frame->byte_count) ||
        !frame_extent(row_bytes, height, destination_stride, capacity))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (y = 0; y < height; ++y)
    {
        memcpy(target + (size_t)y * destination_stride, pixels + (size_t)y * source_stride, row_bytes);
    }
    return FVM_OK;
}
