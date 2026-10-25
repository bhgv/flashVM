/* FlashVM: shared public-region parsing and alias-safe snapshots for logical bitmap operations. */
#ifndef FVM_BITMAP_REGION_HPP
#define FVM_BITMAP_REGION_HPP /* Include guard for clipped pixel regions. */
#include "bitmap_state.hpp"


/* Temporary integer region resolved only after all public coercion callbacks finish. */
struct FVM_BitmapRegion
{
    FVM_Object *source;    /* [GC:] Borrowed source object retained by the caller's native barrier. */
    double box[4];         /* Copied source x/y/width/height before integer clipping. */
    double destination[2]; /* Copied destination point before integer clipping. */
    unsigned sx;           /* First eligible source column after aligned clipping. */
    unsigned sy;           /* First eligible source row after aligned clipping. */
    unsigned dx;           /* Corresponding destination column after aligned clipping. */
    unsigned dy;           /* Corresponding destination row after aligned clipping. */
    unsigned width;        /* Number of eligible columns; zero for an empty region. */
    unsigned height;       /* Number of eligible rows; zero for an empty region. */
    /* Initialize every temporary field before public property coercion. */
    FVM_BitmapRegion();
};


/* [GC:] Copy source/rectangle/point arguments under the caller's native barrier, without pixel
 * borrows. */
FVM_Result fvm_bitmap_region_read(FVM_Context *, unsigned, const FVM_Value *, FVM_BitmapRegion &);


/* [GC:] Refresh receiver/source storage and resolve aligned source/destination clipping after
 * coercion. */
FVM_Result fvm_bitmap_region_clip(FVM_Object *, FVM_BitmapRegion &, FVM_BitmapState *&, FVM_BitmapState *&);


/* Copy a complete eligible pixel region before publication, preserving overlapping source aliases.
 */
FVM_Result fvm_bitmap_region_snapshot(FVM_Context *,
                                      const FVM_BitmapRegion &,
                                      const FVM_BitmapState *,
                                      fvm_detail::vector<unsigned char> &);
#endif
