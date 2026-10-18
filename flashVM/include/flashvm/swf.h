/* FlashVM: Public embedding API declarations: swf.
 * File: include/flashvm/swf.h.
 */

#ifndef FLASHVM_SWF_H
/* Define FLASHVM SWF H constant or compilation switch. */
#define FLASHVM_SWF_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Maximum decoded size for inspection and contexts without a configured limit. */
/* Define FVM SWF DEFAULT LIMIT constant or compilation switch. */
#define FVM_SWF_DEFAULT_LIMIT (64UL * 1024UL * 1024UL)
    /* Define FVM_SWFInfo struct and its representation. */
    typedef struct FVM_SWFInfo
    {
        unsigned long struct_size; /* Size of this structure for API compatibility checks. */
        unsigned int version /* SWF format version from the header. */,
            frame_count /* Number of frames declared by the SWF. */,
            frame_rate_fixed8; /* Frame rate stored in SWF fixed-point format. */
        unsigned long declared_length /* Uncompressed length declared by the SWF header. */,
            tag_count /* Number of parsed SWF tags. */,
            action_tag_count; /* Number of ActionScript action tags. */
    } FVM_SWFInfo;
#define FVM_SWF_TWIPS_PER_PIXEL 20L /* SWF stage coordinates use twenty twips per logical pixel. */
    /* Authored stage rectangle, independent of visible object bounds. */
    typedef struct FVM_SWFStageBounds
    {
        unsigned long struct_size; /* Required sizeof(FVM_SWFStageBounds). */
        long x_min; /* Signed authored left edge in twips. */
        long x_max; /* Signed authored right edge in twips. */
        long y_min; /* Signed authored top edge in twips. */
        long y_max; /* Signed authored bottom edge in twips. */
    } FVM_SWFStageBounds;
    /* Copy the loaded canonical RECT without allocation/script/GC; preserve output on failure. */
    FVM_API FVM_Result fvm_swf_stage_bounds(FVM_Context *, FVM_SWFStageBounds *);
    /* Implement swf inspect for this subsystem. */
    FVM_API FVM_Result fvm_swf_inspect(const void *, size_t, FVM_SWFInfo *);
    /* Implement swf info for this subsystem. */
    FVM_API FVM_Result fvm_swf_info(FVM_Context *, FVM_SWFInfo *);
#ifdef __cplusplus
}
#endif
#endif
