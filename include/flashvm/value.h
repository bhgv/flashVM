#ifndef FLASHVM_VALUE_H
#define FLASHVM_VALUE_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum FVM_ValueType { FVM_VALUE_UNDEFINED=0, FVM_VALUE_NULL=1, FVM_VALUE_BOOLEAN=2, FVM_VALUE_NUMBER=3, FVM_VALUE_STRING=4, FVM_VALUE_OBJECT=5 } FVM_ValueType;
typedef struct FVM_Value {
    FVM_ValueType type;
    double number;
    int boolean;
    const char *string;
    FVM_Object *object;
} FVM_Value;
typedef FVM_Result (*FVM_NativeFunction)(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
FVM_API void fvm_value_undefined(FVM_Value *value);
FVM_API void fvm_value_number(FVM_Value *value, double number);
FVM_API void fvm_value_boolean(FVM_Value *value, int boolean);
FVM_API FVM_Result fvm_value_string(FVM_Value *value, const char *utf8);
FVM_API FVM_Result fvm_object_create(FVM_Context *, FVM_Object **);
/* Prototype lookup follows the chain; own properties shadow inherited ones.
   Cycles and cross-context prototypes are rejected. */
FVM_API FVM_Result fvm_object_set_prototype(FVM_Context *, FVM_Object *, FVM_Object *);
FVM_API FVM_Result fvm_object_get_prototype(FVM_Context *, FVM_Object *, FVM_Object **);
FVM_API FVM_Result fvm_object_has_own(FVM_Context *, FVM_Object *, const char *, int *);
FVM_API FVM_Result fvm_object_get(FVM_Context *, FVM_Object *, const char *, FVM_Value *);
FVM_API FVM_Result fvm_object_set(FVM_Context *, FVM_Object *, const char *, const FVM_Value *);
FVM_API FVM_Result fvm_get_global(FVM_Context *, const char *, FVM_Value *);
FVM_API FVM_Result fvm_set_global(FVM_Context *, const char *, const FVM_Value *);
FVM_API FVM_Result fvm_register_function(FVM_Context *, const char *, FVM_NativeFunction, void *);
/* Registered capture is traced while the function is registered. Replacing
   the function releases this root. Arbitrary userdata memory is never scanned. */
FVM_API FVM_Result fvm_register_function_rooted(FVM_Context *, const char *, FVM_NativeFunction, void *, FVM_Object *);
/* Define/replace an own native method. Capture is traced by GC. Own data or
 * accessor conflicts reject registration. Calls search up to 1024 prototype
 * objects, bind the original receiver, and copy the selected callback entry.
 * Data/accessor properties shadow callbacks; callable properties are not yet
 * dispatched. Replacing a callback drops its previous capture root. */
FVM_API FVM_Result fvm_object_define_method_rooted(FVM_Context *, FVM_Object *,
    const char *, FVM_NativeFunction, void *, FVM_Object *root);
/* Call a native method through the prototype chain, binding self to receiver.
   This is an initial bridge; AVM1 function-valued properties are not supported. */
FVM_API FVM_Result fvm_object_call(FVM_Context *, FVM_Object *, const char *, unsigned int, const FVM_Value *, FVM_Value *);
FVM_API FVM_Result fvm_call(FVM_Context *, const char *, unsigned int, const FVM_Value *, FVM_Value *);
/* Named native construction bridge used by AVM1 ActionNewObject. Object is a
 * reserved built-in (zero arguments). A callback must return a same-context
 * object; arbitrary native functions are never treated as constructors.
 * Registration replacement releases the previous capture root. */
FVM_API FVM_Result fvm_register_constructor_rooted(FVM_Context *, const char *,
    FVM_NativeFunction, void *, FVM_Object *root);
FVM_API FVM_Result fvm_construct(FVM_Context *, const char *, unsigned int,
    const FVM_Value *, FVM_Object **out);
/* Native own accessor; getter receives zero arguments, setter one copied value.
 * Missing getter -> undefined; missing setter -> ignored write. Callbacks bind
 * the original receiver during prototype lookup. Capture root is GC-traced.
 * Data-property conflicts reject registration; an accessor may be replaced.
 * This does not implement AS property attributes/watch/enumeration. */
FVM_API FVM_Result fvm_object_define_accessor_rooted(FVM_Context *, FVM_Object *,
    const char *, FVM_NativeFunction getter, FVM_NativeFunction setter,
    void *, FVM_Object *root);
/* Returned strings and object pointers are borrowed, valid until context destruction.
   Object references cannot cross contexts. Not thread-safe. */
#ifdef __cplusplus
}
#endif
#endif
