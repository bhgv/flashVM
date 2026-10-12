/* FlashVM: managed graph encoding of immutable SWF vector resources. */
#ifndef FVM_SWF_SHAPE_GRAPH_HPP
#define FVM_SWF_SHAPE_GRAPH_HPP /* Include guard for managed shape construction. */
#include "shape.hpp"
#include "../core/runtime.hpp"

/* [GC:] Build a definition whose owned paths/commands survive ordinary graph transport. */
FVM_Result fvm_swf_shape_build(FVM_Context *, const FVM_SWFShape &, FVM_Object **,
                               FVM_Object *bitmap_definitions = 0);
#endif
