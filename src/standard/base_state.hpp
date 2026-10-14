/* FlashVM: Built-in ActionScript objects: base state.
 * File: src/standard/base_state.hpp.
 */

#ifndef FVM_STANDARD_BASE_STATE_HPP
/* Define FVM STANDARD BASE STATE HPP constant or compilation switch. */
#define FVM_STANDARD_BASE_STATE_HPP
#include "../core/runtime.hpp"
/* Build constructor and prototype for base. */
FVM_Result fvm_base_stage(FVM_Context *, fvm_detail::map<fvm_detail::string, FVM_Value> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);

/* [GC:] Prepared Array prototype references remain owned by its constructor root.
 * Stored key addresses let the commit erase descriptors without allocating strings. */
struct FVM_ArrayPrototypeActivation
{
    FVM_Object *prototype; /* Candidate native empty prototype, or null to skip activation. */
    const fvm_detail::string *length_key; /* Native length accessor key retained until commit. */
    const fvm_detail::string *flags_key; /* Matching default length attributes key. */
    /* Start with no candidate or retained descriptor keys. */
    FVM_ArrayPrototypeActivation() : prototype(0), length_key(0), flags_key(0) {}
};
/* Prepare an unchanged host Array prototype without mutation; allocating lookups may throw. */
void fvm_array_prepare_loaded(FVM_Context *, FVM_ArrayPrototypeActivation &);
/* Apply the prepared ordinary-object layout without allocation or callbacks. */
void fvm_array_commit_loaded(const FVM_ArrayPrototypeActivation &);

/* Return the registry backing Object.registerClass in this context. */
FVM_Result fvm_base_class_registry(FVM_Context *, FVM_Object **);

/* Validate base. */
FVM_Result fvm_base_validate(FVM_Context *);

/* Map a native base callback to its stable serialization identifier. */
unsigned long fvm_base_callback_id(FVM_NativeFunction);

/* Restore a native base callback from its stable serialization identifier. */
FVM_NativeFunction fvm_base_callback_from_id(unsigned long);

/* Parse a canonical ActionScript indexed property name. */
bool fvm_as_index(const char *, unsigned long *);

/* Update Array length after writing an indexed property. */
void fvm_array_note_index(FVM_Object *, unsigned long);

/* Read retained stored data from a script-property descriptor without invoking its getter. */
bool fvm_base_accessor_data(const FVM_AccessorEntry &, FVM_Value &);
/* Recognize the script-property bridge whose function setter can run on a readonly property. */
bool fvm_base_script_accessor(const FVM_AccessorEntry &);

/* Recognize the native Array length bridge whose loaded truncation precedes readonly data checks. */
bool fvm_base_array_length_accessor(const FVM_AccessorEntry &);

/* Store own data without setters; note_length optionally suppresses implicit native Array growth. */
FVM_Result fvm_base_store_data(FVM_Context *, FVM_Object *, const fvm_detail::string &, const FVM_Value &,
                               bool note_length = true);

/* Define an own Array result slot; copies strings and does not change the captured length. */
FVM_Result fvm_array_store_own(FVM_Context *, FVM_Object *, unsigned long, const FVM_Value &);

/* Create an Array, optionally selecting the retained builtin result prototype. */
FVM_Result fvm_array_create_like(FVM_Context *, FVM_Object *, unsigned long, FVM_Object **,
                                  bool builtin_result = false);
#endif
