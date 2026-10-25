/* FlashVM: strong graph-owned TextField listeners and prepared notification snapshots. */
#ifndef FVM_STANDARD_TEXT_LISTENERS_HPP
#define FVM_STANDARD_TEXT_LISTENERS_HPP /* Guard private C++98 text event operations. */
#include "../core/runtime.hpp"
#include "../core/object_root_scope.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../core/execution_work_scope.hpp"

#define FVM_TEXT_LISTENER_KEY "\001FlashVM.TextField.listeners" /* Private nonenumerable managed registry. */
#define FVM_TEXT_LISTENER_MAX_COUNT 4096UL                      /* Bounded registration and recipient snapshot work. */
#define FVM_TEXT_LISTENER_MIN_VERSION 6U                        /* TextField listener events begin in SWF6. */
#define FVM_TEXT_LISTENER_RETIRED_CALLBACK 205UL                /* Historical unsupported TextField method body. */
#define FVM_TEXT_LISTENER_CALLBACK_ADD 207UL                    /* Stable text callback graph identity. */
#define FVM_TEXT_LISTENER_CALLBACK_REMOVE 208UL                 /* Stable text callback graph identity. */
/* Register a same-context identity once; publish no partial registry on allocation failure. */
FVM_Result
fvm_textfield_add_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
/* Remove one identity transactionally; report false when absent or invalid script arguments. */
FVM_Result
fvm_textfield_remove_listener(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Validate and borrow the private compact managed Array; NOT_FOUND when never registered. */
FVM_Result fvm_textfield_listener_registry(FVM_Context *, FVM_Object *, FVM_Object **);


/* [GC:] Inspect raw versioned observer/resolver descriptors with charged traversal; never invoke script.
 * Failure leaves out false. The owner serializes prototype mutation and collection. */
FVM_Result fvm_textfield_observer_present(FVM_Context *, FVM_Object *, bool *out);


/* Upgrade only exact retired rootless methods; callers invalidate matching receiver caches. */
bool fvm_textfield_upgrade_listener_method(const char *, FVM_NativeEntry &);


/* [GC:] Root the field and recipient snapshot before publishing an explicit scroll change. */
class FVM_TextFieldScrollPlan
{
    FVM_Context *context_;                     /* Borrowed owner context, alive throughout synchronous delivery. */
    FVM_Object *field_;                        /* [GC:] Source field retained by field_root_. */
    FVM_ObjectRootScope field_root_;           /* [GC:] Source graph survives callback mutation. */
    fvm_detail::vector<FVM_Value> recipients_; /* Copied registration order for this event. */
    FVM_ValueVectorRootScope recipient_roots_; /* [GC:] Snapshot survives live-list removal. */
    bool enabled_;                             /* Older loaded SWF keeps the previous host-only scroll behavior. */
    FVM_ExecutionWorkScope work_;              /* Shared budget through preparation and nested callbacks. */
    bool prepared_;                            /* Delivery requires a complete successful recipient snapshot. */
    FVM_TextFieldScrollPlan(const FVM_TextFieldScrollPlan &);            /* Active roots cannot be copied. */
    FVM_TextFieldScrollPlan &operator=(const FVM_TextFieldScrollPlan &); /* No root reassignment. */

  public:

    /* [GC:] Establish roots and one nested execution budget without allocation or callbacks. */
    FVM_TextFieldScrollPlan(FVM_Context *, FVM_Object *);


    /* Validate/copy recipients before state publication; no script callback occurs. */
    FVM_Result prepare();


    /* Field handler first, then snapshot listeners; first error stops this delivery. */
    FVM_Result dispatch();
};
#endif
