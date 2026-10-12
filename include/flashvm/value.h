#ifndef FLASHVM_VALUE_H
#define FLASHVM_VALUE_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum FVM_ValueType
    {
        FVM_VALUE_UNDEFINED = 0,
        FVM_VALUE_NULL = 1,
        FVM_VALUE_BOOLEAN = 2,
        FVM_VALUE_NUMBER = 3,
        FVM_VALUE_STRING = 4,
        FVM_VALUE_OBJECT = 5
    } FVM_ValueType;
    typedef struct FVM_Value
    {
        FVM_ValueType type;
        double number;
        int boolean;
        const char *string;
        FVM_Object *object;
        /* Per-value metadata for primitives; object values use object identity. */
        FVM_Object *metatable;
        /* Byte length of UTF-8/WTF-8 storage. SIZE_MAX is legacy strlen mode.
         * Counted strings retain embedded NUL and unpaired UTF-16 surrogates. */
        size_t string_size;
    } FVM_Value;
    typedef FVM_Result (*FVM_NativeFunction)(FVM_Context *, FVM_Object *, unsigned int,
                                             const FVM_Value *, FVM_Value *, void *);
    FVM_API void fvm_value_undefined(FVM_Value *value);
    FVM_API void fvm_value_number(FVM_Value *value, double number);
    FVM_API void fvm_value_boolean(FVM_Value *value, int boolean);
    FVM_API FVM_Result fvm_value_string(FVM_Value *value, const char *utf8);
    /* Borrowed counted UTF-8/WTF-8 view; no input terminator is required. */
    FVM_API FVM_Result fvm_value_string_bytes(FVM_Value *, const char *, size_t);
    FVM_API size_t fvm_value_string_size(const FVM_Value *);
    /* Context-owned UTF-16 input; output length counts 16-bit code units.
     * Query with out=NULL/capacity=0; insufficient capacity writes nothing. */
    FVM_API FVM_Result fvm_value_string_utf16(FVM_Context *, const unsigned short *, size_t,
                                              FVM_Value *);
    FVM_API FVM_Result fvm_value_get_utf16(const FVM_Value *, unsigned short *, size_t, size_t *);
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
    FVM_API FVM_Result fvm_register_function(FVM_Context *, const char *, FVM_NativeFunction,
                                             void *);
    /* Registered capture is traced while the function is registered. Replacing
       the function releases this root. Arbitrary userdata memory is never scanned. */
    FVM_API FVM_Result fvm_register_function_rooted(FVM_Context *, const char *, FVM_NativeFunction,
                                                    void *, FVM_Object *);
    /* Define/replace an own native method. Capture is traced by GC. Own data or
     * accessor conflicts reject registration. Calls search up to 1024 prototype
     * objects, bind the original receiver, and copy the selected callback entry.
     * Data/accessor properties shadow native callbacks; function-valued properties
     * dispatch through the Function bridge. Replacing a callback drops its previous capture root.
     */
    FVM_API FVM_Result fvm_object_define_method_rooted(FVM_Context *, FVM_Object *, const char *,
                                                       FVM_NativeFunction, void *,
                                                       FVM_Object *root);
    /* Call a native or script function-valued method through the prototype chain,
       binding self to the original receiver. */
    FVM_API FVM_Result fvm_object_call(FVM_Context *, FVM_Object *, const char *, unsigned int,
                                       const FVM_Value *, FVM_Value *);
    FVM_API FVM_Result fvm_call(FVM_Context *, const char *, unsigned int, const FVM_Value *,
                                FVM_Value *);
    /* Named native construction bridge used by AVM1 ActionNewObject. Without standard installation
     * Object is a reserved zero-argument built-in; installed constructors use their AS APIs. A
     * callback must return a same-context object; arbitrary native functions are never treated as
     * constructors. Registration replacement releases the previous capture root. */
    FVM_API FVM_Result fvm_register_constructor_rooted(FVM_Context *, const char *,
                                                       FVM_NativeFunction, void *,
                                                       FVM_Object *root);
    FVM_API FVM_Result fvm_construct(FVM_Context *, const char *, unsigned int, const FVM_Value *,
                                     FVM_Object **out);
    /* Native own accessor; getter receives zero arguments, setter one copied value.
     * Missing getter -> undefined; missing setter -> ignored write. Callbacks bind
     * the original receiver during prototype lookup. Capture root is GC-traced.
     * Data-property conflicts reject registration; an accessor may be replaced.
     * Property flags are exposed in property.h; accessor properties do not invoke watch handlers.
     */
    FVM_API FVM_Result fvm_object_define_accessor_rooted(FVM_Context *, FVM_Object *, const char *,
                                                         FVM_NativeFunction getter,
                                                         FVM_NativeFunction setter, void *,
                                                         FVM_Object *root);
/* Returned strings and object pointers are borrowed, valid until context destruction.
   Object references cannot cross contexts. Not thread-safe. */
#ifdef __cplusplus
}
#endif
#endif
