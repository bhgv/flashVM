/* FlashVM: managed BlurFilter parameters, constructor identity and portable graph callbacks. */
#ifndef FVM_BLUR_HPP
#define FVM_BLUR_HPP /* Include guard for blur filter integration. */
#include "filter_state.hpp"
#include "bitmap_region.hpp"
/* Append-only graph identities for blur instances and classes. */
enum FVM_BlurKind
{
    FVM_BLUR_FILTER = 60,      /* BlurFilter instance with private numeric parameters. */
    FVM_BLUR_FILTER_CLASS = 61 /* Callable BlurFilter constructor identity. */
};
/* [GC:] Copy three validated private options without public property execution. */
bool fvm_blur_values(FVM_Context *, FVM_Object *, double *);
/* [GC:] Stage blur constructor/prototype/factory edges before publishing the filter namespace. */
FVM_Result fvm_blur_stage(FVM_Context *, FVM_Object *, FVM_Object *, FVM_Object *,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
/* Check the installed constructor identity without running user accessors. */
bool fvm_blur_validate(FVM_Context *, FVM_Object *);
/* Identify stable blur callback IDs for graph transport. */
unsigned long fvm_blur_callback_id(FVM_NativeFunction);
/* Restore only known blur callbacks without global installation. */
FVM_NativeFunction fvm_blur_callback_from_id(unsigned long);
/* Compute the discrete nonzero support radius of one fractional box pass. */
unsigned fvm_blur_radius(double);
/* [GC:] Apply copied blur parameters to padded premultiplied staging planes before publication. */
FVM_Result fvm_blur_apply(FVM_Context *, FVM_Object *, const FVM_BitmapRegion &, const double *,
                          FVM_Value *);
#endif
