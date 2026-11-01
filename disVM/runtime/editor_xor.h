/* Destination-dependent temporary outline rasterization on detached surfaces. */
#ifndef DVM_EDITOR_XOR_H
#define DVM_EDITOR_XOR_H
#include "flashvm/path.h"
#include <stdlib.h>


/* Draw an outline coverage mask, then invert covered RGB bytes once. Borrow
 * immutable geometry and exclusive destination; alpha is preserved. Thresholded
 * coverage makes repeated application exactly reversible. Allocation/raster
 * failure leaves the destination unchanged. No VM or shared state is accessed. */
static FVM_Result dvm_editor_xor_draw(const FVM_Path *path, FVM_RasterSurface *surface, double width)
{
    FVM_RasterSurface mask = *surface;
    FVM_RasterColor white = {255, 255, 255, 255};
    FVM_Result result;
    unsigned int x;
    unsigned int y;
    mask.stride = (size_t)mask.width * 4;
    mask.pixels = calloc(mask.height, mask.stride);
    if (!mask.pixels)
    {
        return FVM_OUT_OF_MEMORY;
    }
    result = fvm_path_draw(path, &mask, white, width, 0);
    if (result == FVM_OK)
    {
        for (y = 0; y < mask.height; ++y)
        {
            for (x = 0; x < mask.width; ++x)
            {
                if (mask.pixels[(size_t)y * mask.stride + x * 4 + 3] >= 128)
                {
                    unsigned char *pixel = surface->pixels + (size_t)y * surface->stride + x * 4;
                    pixel[0] ^= 255;
                    pixel[1] ^= 255;
                    pixel[2] ^= 255;
                }
            }
        }
    }
    free(mask.pixels);
    return result;
}
#endif
