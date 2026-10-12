/* FlashVM: Public embedding API declarations: value.
 * File: include/flashvm/value.h.
 */

#ifndef FLASHVM_VALUE_H
/* Define FLASHVM VALUE H constant or compilation switch. */
#define FLASHVM_VALUE_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Define FVM_ValueType enum and its representation. */
    typedef enum FVM_ValueType
    {
        FVM_VALUE_UNDEFINED = 0 /* Undefined ActionScript value. */,
        FVM_VALUE_NULL = 1 /* Null ActionScript value. */,
        FVM_VALUE_BOOLEAN = 2 /* Boolean ActionScript value. */,
        FVM_VALUE_NUMBER = 3 /* Numeric ActionScript value. */,
        FVM_VALUE_STRING = 4 /* String ActionScript value. */,
        FVM_VALUE_OBJECT = 5 /* Managed object value. */
    } FVM_ValueType;
    /* Define FVM_Value struct and its representation. */
    typedef struct FVM_Value
    {
        FVM_ValueType type; /* Tag selecting the active value representation. */
        double number;      /* Numeric payload of the value. */
        int boolean;        /* Boolean payload of the value. */
        const char *string; /* Pointer to string storage; ownership follows the enclosing API. */
        FVM_Object *object; /* Reference to a managed object. */
        /* Per-value metadata for primitives; object values use object identity. */
        FVM_Object *metatable; /* Metadata object used for metamethod dispatch. */
        /* Byte length of UTF-8/WTF-8 storage. SIZE_MAX is legacy strlen mode.
         * Counted strings retain embedded NUL and unpaired UTF-16 surrogates. */
        size_t string_size; /* Stored byte count, or SIZE_MAX for legacy terminated strings. */
    } FVM_Value;
    typedef FVM_Result (*FVM_NativeFunction)(FVM_Context *, FVM_Object *, unsigned int,
                                             const FVM_Value *, FVM_Value *, void *);
    /* Initialize every value field to the undefined representation. */
    FVM_API void fvm_value_undefined(FVM_Value *value);
    /* Initialize a numeric value with the supplied double payload. */
    FVM_API void fvm_value_number(FVM_Value *value, double number);
    /* Initialize a Boolean value from the supplied truth value. */
    FVM_API void fvm_value_boolean(FVM_Value *value, int boolean);
    /* Initialize a borrowed terminated UTF-8/WTF-8 string value. */
    FVM_API FVM_Result fvm_value_string(FVM_Value *value, const char *utf8);
    /* Borrowed counted UTF-8/WTF-8 view; no input terminator is required. */
    /* Initialize a borrowed counted string, preserving embedded zero bytes. */
    FVM_API FVM_Result fvm_value_string_bytes(FVM_Value *, const char *, size_t);
    /* Return the stored string byte count, resolving legacy terminated input. */
    FVM_API size_t fvm_value_string_size(const FVM_Value *);
    /* Context-owned UTF-16 input; output length counts 16-bit code units.
     * Query with out=NULL/capacity=0; insufficient capacity writes nothing. */
    /* Convert UTF-16 code units into context-owned WTF-8 string storage. */
    FVM_API FVM_Result fvm_value_string_utf16(FVM_Context *, const unsigned short *, size_t,
                                              FVM_Value *);
    /* Query or copy the UTF-16 code units represented by this string value. */
    FVM_API FVM_Result fvm_value_get_utf16(const FVM_Value *, unsigned short *, size_t, size_t *);
    /* Create object. */
    FVM_API FVM_Result fvm_object_create(FVM_Context *, FVM_Object **);
    /* Prototype lookup follows the chain; own properties shadow inherited ones.
       Cycles and cross-context prototypes are rejected. */
    /* Update object prototype. */
    FVM_API FVM_Result fvm_object_set_prototype(FVM_Context *, FVM_Object *, FVM_Object *);
    /* Read object prototype. */
    FVM_API FVM_Result fvm_object_get_prototype(FVM_Context *, FVM_Object *, FVM_Object **);
    /* Test presence of object own. */
    FVM_API FVM_Result fvm_object_has_own(FVM_Context *, FVM_Object *, const char *, int *);
    /* Read a named property, following accessors and the prototype chain. */
    FVM_API FVM_Result fvm_object_get(FVM_Context *, FVM_Object *, const char *, FVM_Value *);
    /* Assign a named property using attributes, accessors and watchers. */
    FVM_API FVM_Result fvm_object_set(FVM_Context *, FVM_Object *, const char *, const FVM_Value *);
    /* Read global. */
    FVM_API FVM_Result fvm_get_global(FVM_Context *, const char *, FVM_Value *);
    /* Update global. */
    FVM_API FVM_Result fvm_set_global(FVM_Context *, const char *, const FVM_Value *);
    /* Register function. */
    FVM_API FVM_Result fvm_register_function(FVM_Context *, const char *, FVM_NativeFunction,
                                             void *);
    /* [GC:] Registered capture is traced while the function is registered. Replacing
       the function releases this root. Arbitrary userdata memory is never scanned. */
    /* [GC:] Register function rooted. */
    FVM_API FVM_Result fvm_register_function_rooted(FVM_Context *, const char *, FVM_NativeFunction,
                                                    void *, FVM_Object *);
    /* [GC:] Define/replace an own native method. Capture is traced by GC. Own data or
     * accessor conflicts reject registration. Calls search up to 1024 prototype
     * objects, bind the original receiver, and copy the selected callback entry.
     * Data/accessor properties shadow native callbacks; function-valued properties
     * dispatch through the Function bridge. Replacing a callback drops its previous capture root.
     */
    /* [GC:] Define object method rooted. */
    FVM_API FVM_Result fvm_object_define_method_rooted(
        FVM_Context *, FVM_Object *, const char *,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction, void *,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root);
    /* Call a native or script function-valued method through the prototype chain,
       binding self to the original receiver. */
    /* Call object. */
    FVM_API FVM_Result fvm_object_call(FVM_Context *, FVM_Object *, const char *, unsigned int,
                                       const FVM_Value *, FVM_Value *);
    /* Call the current value. */
    FVM_API FVM_Result fvm_call(FVM_Context *, const char *, unsigned int, const FVM_Value *,
                                FVM_Value *);
    /* [GC:] Named native construction bridge used by AVM1 ActionNewObject. Without standard
     * installation Object is a reserved zero-argument built-in; installed constructors use their AS
     * APIs. A callback must return a same-context object; arbitrary native functions are never
     * treated as constructors. Registration replacement releases the previous capture root. */
    /* [GC:] Register constructor rooted. */
    FVM_API FVM_Result fvm_register_constructor_rooted(
        FVM_Context *, const char *,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction, void *,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root);
    /* Implement construct for this subsystem. */
    FVM_API FVM_Result fvm_construct(FVM_Context *, const char *, unsigned int, const FVM_Value *,
                                     FVM_Object **out);
    /* [GC:] Native own accessor; getter receives zero arguments, setter one copied value.
     * Missing getter -> undefined; missing setter -> ignored write. Callbacks bind
     * the original receiver during prototype lookup. Capture root is GC-traced.
     * Data-property conflicts reject registration; an accessor may be replaced.
     * Property flags are exposed in property.h; accessor properties do not invoke watch handlers.
     */
    /* [GC:] Define object accessor rooted. */
    FVM_API FVM_Result fvm_object_define_accessor_rooted(
        FVM_Context *, FVM_Object *, const char *,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction getter,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_NativeFunction setter, void *,
        /* [GC:] Операция в блоке управления достижимостью и временем жизни объектов. */
        FVM_Object *root);

/* Returned strings and object pointers are borrowed, valid until context destruction.
   Object references cannot cross contexts. Not thread-safe. */
#ifdef __cplusplus
}
#endif
#endif
