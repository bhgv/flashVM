/* FlashVM: SWF file parsing and model access: swf.
 * File: src/swf/swf_c.cpp.
 */

#include "../core/runtime.hpp"
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
