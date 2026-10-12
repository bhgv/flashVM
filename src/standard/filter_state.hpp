/* FlashVM: managed Flash 8 bitmap filter classes and copied color matrix state. */
#ifndef FVM_FILTER_STATE_HPP
#define FVM_FILTER_STATE_HPP /* Include guard for managed filter integration. */
#include "geometry_support.hpp"
/* Append-only graph resource kinds for filter instances and constructor identities. */
enum FVM_FilterKind
{
    FVM_COLOR_MATRIX_FILTER = 54, /* ColorMatrixFilter with private numeric coefficient storage. */
    FVM_COLOR_MATRIX_FILTER_CLASS = 55, /* Callable ColorMatrixFilter constructor identity. */
    FVM_BITMAP_FILTER = 56,             /* Empty BitmapFilter base instance. */
    FVM_BITMAP_FILTER_CLASS = 57        /* Callable BitmapFilter base constructor identity. */
};
/* [GC:] Stage filter classes, inheritance and captured Array factories before publishing flash. */
FVM_Result fvm_filter_stage(FVM_Context *, FVM_Object *, FVM_Object *, FVM_Object *, FVM_Object *,
                            FVM_Object *, fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
/* Validate filter namespace identities without public getter dispatch. */
bool fvm_filter_validate(FVM_Context *, FVM_Object *);
/* [GC:] Copy validated private coefficients without exposing or retaining their managed state. */
bool fvm_filter_matrix(FVM_Context *, FVM_Object *, double *);
/* Identify native filter functions using append-only transport identifiers. */
unsigned long fvm_filter_callback_id(FVM_NativeFunction);
/* Restore only a recognized filter implementation from its transport identifier. */
FVM_NativeFunction fvm_filter_callback_from_id(unsigned long);
/* [GC:] Apply a copied color matrix to an alias-safe source snapshot. */
FVM_Result fvm_bitmap_apply_filter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                   FVM_Value *, void *);
/* [GC:] Return an independent typed filter output rectangle using retained geometry factories. */
FVM_Result fvm_bitmap_filter_rect(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                  FVM_Value *, void *);
#endif
