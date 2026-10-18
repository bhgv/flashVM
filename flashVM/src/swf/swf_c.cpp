/* FlashVM: SWF file parsing and model access: swf.
 * File: src/swf/swf_c.cpp.
 */

#include "../core/runtime.hpp"
#include "reader.hpp"
#include "stage_constants.h"
/* Compute and return swf info. */
extern "C" FVM_Result fvm_swf_info(FVM_Context *context, FVM_SWFInfo *info)
{
    if (!context || !info || info->struct_size != sizeof(*info))
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return FVM_UNSUPPORTED;
    *info = context->swf_info;
    return FVM_OK;
}

/* Read authored coordinates from canonical FWS bytes, including contexts loaded from CWS.
 * [GC:] No allocation, callback or retained view; owning-thread/nonterminal calls only. */
extern "C" FVM_Result fvm_swf_stage_bounds(FVM_Context *context, FVM_SWFStageBounds *out)
{
    if (!context || !out || out->struct_size != sizeof(*out) || context->destroying ||
        context->gc_collecting || context->gc_releasing)
        return FVM_INVALID_ARGUMENT;
    if (!context->swf_loaded)
        return FVM_UNSUPPORTED;
    if (context->swf_bytes.size() <= FVM_SWF_STAGE_RECT_OFFSET)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFReader reader(context->swf_bytes.begin() + FVM_SWF_STAGE_RECT_OFFSET,
                         context->swf_bytes.size() - FVM_SWF_STAGE_RECT_OFFSET);
    unsigned int width = (unsigned int)reader.bits(FVM_SWF_STAGE_WIDTH_BITS);
    if (!width)
        return FVM_INVALID_ARGUMENT;
    FVM_SWFStageBounds value;
    value.struct_size = sizeof(value);
    value.x_min = reader.signed_bits(width);
    value.x_max = reader.signed_bits(width);
    value.y_min = reader.signed_bits(width);
    value.y_max = reader.signed_bits(width);
    if (!reader.good() || value.x_max < value.x_min || value.y_max < value.y_min)
        return FVM_INVALID_ARGUMENT;
    *out = value;
    return FVM_OK;
}
