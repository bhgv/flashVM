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
FVM_API FVM_Result fvm_call(FVM_Context *, const char *, unsigned int, const FVM_Value *, FVM_Value *);
/* Returned strings and object pointers are borrowed, valid until context destruction.
   Object references cannot cross contexts. Not thread-safe. */
#ifdef __cplusplus
}
#endif
#endif
