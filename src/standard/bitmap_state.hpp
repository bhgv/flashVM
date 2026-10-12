/* FlashVM: owned BitmapData pixel storage, static-class installation and binary snapshots. */
#ifndef FVM_BITMAP_STATE_HPP
#define FVM_BITMAP_STATE_HPP /* Include guard for managed bitmap integration. */
#include "geometry_support.hpp"
/* [GC:] Owned pixel allocation released by the object's internal resource destructor. */
struct FVM_BitmapState
{
    unsigned int width;  /* Pixel width; zero after disposal. */
    unsigned int height; /* Pixel height; zero after disposal. */
    bool transparent;    /* Whether alpha bytes are retained. */
    bool disposed;       /* Whether pixel operations are invalid. */
    fvm_detail::vector<unsigned char>
        pixels; /* Straight RGBA bytes owned independently of script strings. */
    /* Initialize an invalid empty allocation before construction or restoration. */
    FVM_BitmapState() : width(0), height(0), transparent(true), disposed(true)
    {
    }
};
/* [GC:] Stage BitmapData under flash.display and retain geometry factory classes. */
FVM_Result fvm_bitmap_stage(FVM_Context *, FVM_Object *, FVM_Object *, FVM_Object *, FVM_Object *,
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);
/* Validate the staged/installed display namespace without firing user accessors. */
bool fvm_bitmap_validate(FVM_Context *, FVM_Object *);
/* Identify a native bitmap callback by a stable transport ID. */
unsigned long fvm_bitmap_callback_id(FVM_NativeFunction);
/* Restore a known native bitmap callback without global installation. */
FVM_NativeFunction fvm_bitmap_callback_from_id(unsigned long);
/* Copy canonical binary pixel state into graph-owned bytes. */
FVM_Result fvm_bitmap_snapshot(FVM_Object *, fvm_detail::string &);
/* Validate canonical dimensions, flags and exact pixel length before allocation. */
bool fvm_bitmap_snapshot_valid(const fvm_detail::string &);
/* [GC:] Attach independently allocated native pixel storage to a restored object. */
FVM_Result fvm_bitmap_restore(FVM_Object *, const fvm_detail::string &);
#endif
