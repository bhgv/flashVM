/* FlashVM: Public embedding API declarations: movieclip.
 * File: include/flashvm/movieclip.h.
 */

#ifndef FLASHVM_MOVIECLIP_H
/* Define FLASHVM MOVIECLIP H constant or compilation switch. */
#define FLASHVM_MOVIECLIP_H
#include "value.h"
#include "raster.h"
#include "path.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Neutral SWF document metadata; strings are borrowed from the rooted clip graph. */
    typedef struct FVM_SWFDocumentInfo
    {
        unsigned long struct_size; /* Exact structure size required by this API. */
        int file_attributes_present; /* Distinguish absent attributes from explicit zero flags. */
        unsigned long file_attributes; /* Supported flags are zero or HasMetadata (0x10). */
        FVM_Value metadata; /* Undefined if absent; otherwise counted context-owned UTF-8. */
    } FVM_SWFDocumentInfo;
    /* Read without script execution; errors leave output unchanged. [GC:] Root the clip or retain
       the returned string before collection. Metadata is not parsed or executed as XML. */
    FVM_API FVM_Result fvm_movieclip_swf_document_info(FVM_Context *, FVM_Object *,
                                                       FVM_SWFDocumentInfo *);
    /* Create movieclip. */
    FVM_API FVM_Result fvm_movieclip_create(FVM_Context *, const char *name, unsigned int frames,
                                            FVM_Object **);
    /* Import root/sprite DoAction/FrameLabel timelines and ExportAssets from SWF 1..8.
       FWS/CWS decoding is bounded; unimplemented scene/resource tags return FVM_UNSUPPORTED.
       Loading runs no actions. Exported sprites can be instantiated through attachMovie.
       On failure *out is null. [GC:] The returned clip and resource library must be rooted by their
       host owner. */
    FVM_API FVM_Result fvm_movieclip_load_swf(FVM_Context *, const void *data, size_t size,
                                              const char *name, FVM_Object **out);
    /* Query the local affine transform in pixels, preserving authored shear/reflection. */
    FVM_API FVM_Result fvm_movieclip_get_matrix(FVM_Context *, FVM_Object *, FVM_Affine *);
    /* Atomically set a finite local transform and corresponding display components. */
    FVM_API FVM_Result fvm_movieclip_set_matrix(FVM_Context *, FVM_Object *, const FVM_Affine *);
    /* Copy a validated AVM1 frame action and optional label. Frames start at 1. */
    /* Define movieclip frame. */
    FVM_API FVM_Result fvm_movieclip_define_frame(FVM_Context *, FVM_Object *, unsigned int,
                                                  const char *label, const void *actions, size_t);
    /* Implement movieclip advance for this subsystem. */
    FVM_API FVM_Result fvm_movieclip_advance(FVM_Context *, FVM_Object *, unsigned int ticks);
    /* Render movieclip. */
    FVM_API FVM_Result fvm_movieclip_render(FVM_Context *, FVM_Object *, FVM_RasterSurface *);
#ifdef __cplusplus
}
#endif
#endif
