/* Generic detached affine/marquee modifiers, no VM objects or input policy. */
#ifndef DVM_VECTOR_TRANSFORM_H
#define DVM_VECTOR_TRANSFORM_H
#include "vector_local.h"


/* Transform a complete copied path/decorations into private output arrays.
 * plan defines the immutable pointer origin, modifier mode and scale corner.
 * Return0 on out-of-page source coordinates; decorations clamp to page edges.
 * Caller publishes only on success. No allocation, callbacks or threads. */
int dvm_vector_transform(const DVM_VectorLocalPlan *plan,
    const DVM_VectorCommand *source, const DVM_VectorCommand *overlay,
    long x, long y, DVM_VectorCommand *output, DVM_VectorCommand *decorated);
#endif
