/* Backend-neutral borrowed frame boundary and affine pointer mapping. */
#include "flashvm/host.h"
#include <cfloat>
#include <cmath>
#include <new>

/* Reject NaN/infinity without relying on post-C++98 math classification. */
static bool host_finite(double value)
{
    return value >= -DBL_MAX && value <= DBL_MAX;
}

/* Check buffer arithmetic before entering a host library or touching frame bytes. */
extern "C" FVM_Result fvm_host_present(const FVM_RasterSurface *surface, size_t byte_count,
                                        FVM_HostPresentCallback callback, void *userdata)
{
    if (!surface || !callback || !surface->pixels || !surface->width || !surface->height)
        return FVM_INVALID_ARGUMENT;
    const size_t maximum = static_cast<size_t>(-1);
    if (surface->width > maximum / FVM_HOST_RGBA_BYTES)
        return FVM_INVALID_ARGUMENT;
    const size_t row = static_cast<size_t>(surface->width) * FVM_HOST_RGBA_BYTES;
    if (surface->stride < row || (surface->height - 1) > (maximum - row) / surface->stride)
        return FVM_INVALID_ARGUMENT;
    const size_t required = static_cast<size_t>(surface->height - 1) * surface->stride + row;
    if (byte_count < required)
        return FVM_INVALID_ARGUMENT;
    /* Copy metadata so the consumer never sees a descriptor it can mutate via an alias. */
    const FVM_RasterSurface frame = *surface;
    try { return callback(&frame, userdata); }
    catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
    catch (...) { return FVM_ERROR; }
}

/* Normalize linear coefficients before inversion to avoid determinant overflow. */
extern "C" FVM_Result fvm_host_pointer_to_world(const FVM_Affine *matrix,
                                                 double host_x, double host_y,
                                                 double *world_x, double *world_y)
{
    if (!matrix || !world_x || !world_y || world_x == world_y ||
        !host_finite(host_x) || !host_finite(host_y) ||
        !host_finite(matrix->xx) || !host_finite(matrix->xy) ||
        !host_finite(matrix->yx) || !host_finite(matrix->yy) ||
        !host_finite(matrix->x0) || !host_finite(matrix->y0))
        return FVM_INVALID_ARGUMENT;
    double scale = std::fabs(matrix->xx);
    if (std::fabs(matrix->xy) > scale) scale = std::fabs(matrix->xy);
    if (std::fabs(matrix->yx) > scale) scale = std::fabs(matrix->yx);
    if (std::fabs(matrix->yy) > scale) scale = std::fabs(matrix->yy);
    if (!scale) return FVM_INVALID_ARGUMENT;
    const double a = matrix->xx / scale, b = matrix->xy / scale;
    const double c = matrix->yx / scale, d = matrix->yy / scale;
    const double determinant = a * d - b * c;
    if (!determinant) return FVM_INVALID_ARGUMENT;
    const double x = (host_x - matrix->x0) / scale;
    const double y = (host_y - matrix->y0) / scale;
    if (!host_finite(x) || !host_finite(y)) return FVM_INVALID_ARGUMENT;
    const double mapped_x = (d * x - b * y) / determinant;
    const double mapped_y = (a * y - c * x) / determinant;
    if (!host_finite(mapped_x) || !host_finite(mapped_y)) return FVM_INVALID_ARGUMENT;
    *world_x = mapped_x;
    *world_y = mapped_y;
    return FVM_OK;
}
