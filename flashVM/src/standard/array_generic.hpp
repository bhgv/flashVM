/* FlashVM: Built-in ActionScript objects: array generic.
 * File: src/standard/array_generic.hpp.
 */

#ifndef FVM_ARRAY_GENERIC_HPP
/* Define FVM ARRAY GENERIC HPP constant or compilation switch. */
#define FVM_ARRAY_GENERIC_HPP
#include "../core/runtime.hpp"


/* Implement array needs generic for this subsystem. */
bool fvm_array_needs_generic(FVM_Object *);


/* Store a loaded own element without callbacks, applying native length growth before data flags. */
FVM_Result fvm_array_legacy_store_element(FVM_Context *, FVM_Object *, unsigned long, const FVM_Value &);

/* Read loaded native Array own length as Int32; errors preserve output. */


/* [GC:] Retains a coercible stored object through callbacks without invoking length getters. */
FVM_Result fvm_array_legacy_length(FVM_Context *, FVM_Object *, long *);


/* Apply raw native Int32 length without setter/watch callbacks, preserving truncation errors. */
FVM_Result fvm_array_legacy_set_length(FVM_Context *, FVM_Object *, long);

/* Prepare exact Array length writes before flags and live setter selection. */


/* [GC:] The caller retains receiver and input across conversion and descriptor mutation. */
FVM_Result fvm_array_prepare_length_data(FVM_Context *, FVM_Object *, const FVM_Value &, bool *);


/* Captured splice preparation has no managed edges or callback-owned values. */
struct FVM_ArraySpliceBounds
{
    unsigned long length;  /* Source length captured between the two conversions. */
    long negative_length;  /* Captured negative borrowed length; zero selects ordinary bounds. */
    unsigned long start;   /* Absolute start computed against the captured length. */
    unsigned long deleted; /* Number of slots selected for removal. */
    unsigned int added;    /* Number of insertion arguments. */
    bool rejected;         /* Missing/undefined/negative arguments produce undefined. */


    /* Initialize a rejected, empty preparation before any observable callback. */
    FVM_ArraySpliceBounds() : length(0), negative_length(0), start(0), deleted(0), added(0), rejected(true)
    {
    }
};

/* Convert start, capture length and convert deletion exactly once, in that order. */
FVM_Result
fvm_array_splice_prepare(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_ArraySpliceBounds &);


/* Run property-aware methods with optional splice bounds and caller-owned fast preparation. */
FVM_Result fvm_array_generic(FVM_Context *,
                             FVM_Object *,
                             const char *,
                             unsigned int,
                             const FVM_Value *,
                             FVM_Value *,
                             FVM_NativeFunction,
                             const FVM_ArraySpliceBounds *prepared = 0,
                             void *fast_data = 0);
#endif
