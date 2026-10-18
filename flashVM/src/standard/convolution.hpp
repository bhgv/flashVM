/* FlashVM: copied ConvolutionFilter parameters and managed filter class integration. */
#ifndef FVM_CONVOLUTION_HPP
#define FVM_CONVOLUTION_HPP /* Include guard for convolution filters. */
#include "filter_state.hpp"
#include "bitmap_region.hpp"
/* Append-only identities used by filter graph transport. */
enum FVM_ConvolutionKind
{
    FVM_CONVOLUTION_FILTER = 58, /* ConvolutionFilter instance with managed private numeric data. */
    FVM_CONVOLUTION_FILTER_CLASS = 59 /* Callable ConvolutionFilter class identity. */
};
/* Parameter positions shared by native properties, validation and pixel processing. */
enum FVM_ConvolutionParameter
{
    FVM_CONVOLUTION_X = 0, /* Kernel columns in 0..15. */
    FVM_CONVOLUTION_Y = 1, /* Kernel rows in 0..15. */
    FVM_CONVOLUTION_DIVISOR =
        2,                    /* Divisor; zero selects the default divisor one when processing. */
    FVM_CONVOLUTION_BIAS = 3, /* Constant added to each filtered channel. */
    FVM_CONVOLUTION_PRESERVE = 4, /* Whether source alpha is retained instead of convolved. */
    FVM_CONVOLUTION_CLAMP = 5,    /* Whether out-of-image taps repeat the closest edge pixel. */
    FVM_CONVOLUTION_COLOR = 6,    /* Low 24-bit substitute RGB color outside the source image. */
    FVM_CONVOLUTION_ALPHA = 7     /* Substitute alpha fraction in 0..1. */
};
/* A bounded detached copy; no managed object or live pixel pointers escape validation. */
struct FVM_Convolution
{
    double parameters[8]; /* Copied dimensions, arithmetic coefficients and edge options. */
    unsigned length;      /* Number of stored row-major kernel coefficients, at most 225. */
    double matrix[225];   /* Copied coefficients; unused entries are initialized to zero. */
    /* Initialize the documented empty-kernel defaults before public conversions. */
    FVM_Convolution();
};
/* [GC:] Copy validated private convolution storage while the caller protects its owning filter. */
bool fvm_convolution_values(FVM_Context *, FVM_Object *, FVM_Convolution &);
/* [GC:] Add ConvolutionFilter to the unpublished filter namespace and captured class graph. */
FVM_Result fvm_convolution_stage(FVM_Context *, FVM_Object *, FVM_Object *, FVM_Object *,
                                 fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
/* Validate the installed class identity without running script properties. */
bool fvm_convolution_validate(FVM_Context *, FVM_Object *);
/* Identify append-only convolution callback IDs for graph transport. */
unsigned long fvm_convolution_callback_id(FVM_NativeFunction);
/* Restore a recognized native convolution callback without global standard installation. */
FVM_NativeFunction fvm_convolution_callback_from_id(unsigned long);
/* [GC:] Apply detached convolution parameters after public region conversion and storage refresh.
 */
FVM_Result fvm_convolution_apply(FVM_Context *, FVM_Object *, const FVM_BitmapRegion &,
                                 const FVM_Convolution &, FVM_Value *);
#endif
