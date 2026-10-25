/* FlashVM: Built-in ActionScript objects: date state.
 * File: src/standard/date_state.hpp.
 */

#ifndef FVM_STANDARD_DATE_STATE_HPP
/* Define FVM STANDARD DATE STATE HPP constant or compilation switch. */
#define FVM_STANDARD_DATE_STATE_HPP
#include "../core/runtime.hpp"


/* Build constructor and prototype for date. */
FVM_Result fvm_date_stage(FVM_Context *,
                          fvm_detail::map<fvm_detail::string, FVM_Value> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &,
                          fvm_detail::map<fvm_detail::string, FVM_NativeEntry> &);


/* [GC:] Staged Date layout and attributes remain retained by the constructor capture. */
struct FVM_DatePrototypeActivation
{
    FVM_Object *attribute_prototype; /* [GC:] Native prototype retained for independent method flags. */
    FVM_Object *prototype;           /* Unchanged invalid native prototype selected for activation. */
    FVM_Object *klass;               /* Captured Date class receiving default UTC attributes. */
    fvm_detail::map<fvm_detail::string, unsigned int> prototype_attributes; /* Prepared method flags. */
    fvm_detail::map<fvm_detail::string, unsigned int> class_attributes;     /* Prepared static flags. */


    /* Start without a candidate; temporary maps own only names and numeric flags. */
    FVM_DatePrototypeActivation() : attribute_prototype(0), prototype(0), klass(0)
    {
    }
};


/* Prepare class flags independently, then any invalid host prototype; may throw. */
void fvm_date_prepare_loaded(FVM_Context *, FVM_DatePrototypeActivation &);


/* Commit class flags and any prepared native state without allocation or callbacks. */
void fvm_date_commit_loaded(FVM_DatePrototypeActivation &);


/* Validate date. */
FVM_Result fvm_date_validate(FVM_Context *);


/* Map a native date callback to its stable serialization identifier. */
unsigned long fvm_date_callback_id(FVM_NativeFunction);


/* Restore a native date callback from its stable serialization identifier. */
FVM_NativeFunction fvm_date_callback_from_id(unsigned long);
#endif
