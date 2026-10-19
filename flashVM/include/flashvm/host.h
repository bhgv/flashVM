/* Backend-independent frame delivery and host-to-world coordinate conversion. */
#ifndef FLASHVM_HOST_H
#define FLASHVM_HOST_H
#include "path.h"
#ifdef __cplusplus
extern "C" {
#endif
#define FVM_HOST_RGBA_BYTES 4U /* Straight-alpha RGBA8 bytes in every public frame. */
/* Synchronous host consumer. Copy/upload the borrowed frame before returning;
 * do not retain its descriptor or pixels. Return the actual backend status. */
typedef FVM_Result (*FVM_HostPresentCallback)(const FVM_RasterSurface *surface, void *userdata);
/* Validate a nonempty frame against its actual accessible byte count, then call
 * the consumer exactly once. Row padding is allowed; the final row need only
 * contain width*4 bytes. No context, allocation, OS, SDL or GPU dependency.
 * Invalid arguments/overflow return INVALID_ARGUMENT without invoking callback.
 * Callback exceptions become OUT_OF_MEMORY (bad_alloc) or ERROR (others).
 * Calls are synchronous on the caller's thread; backend thread rules still apply. */
FVM_API FVM_Result fvm_host_present(const FVM_RasterSurface *surface, size_t byte_count,
                                    FVM_HostPresentCallback callback, void *userdata);
/* Invert the host's world-to-event-space affine transform for one event.
 * Include stage origin, device scale, letterbox and actor transform exactly once.
 * Outputs must be non-NULL and distinct; errors preserve both. Finite nonsingular
 * input and representable intermediate/results are required, else INVALID_ARGUMENT.
 * No clipping, callbacks, context mutation, allocation or contact arbitration.
 * The resulting world pixels can be passed to fvm_input_dispatch_mouse. */
FVM_API FVM_Result fvm_host_pointer_to_world(const FVM_Affine *world_to_host,
                                             double host_x, double host_y,
                                             double *world_x, double *world_y);
#ifdef __cplusplus
}
#endif
#endif
