#ifndef FLASHVM_STACK_H
#define FLASHVM_STACK_H
#include "value.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Lua-inspired, independent argument stack. Not a Lua ABI. A stack belongs
 * to exactly one context and must not be accessed concurrently. */
typedef struct FVM_Stack FVM_Stack;
typedef FVM_Result (*FVM_StackFunction)(FVM_Context *, FVM_Stack *, void *);
FVM_API FVM_Result fvm_stack_create(FVM_Context *, FVM_Stack **);
FVM_API void fvm_stack_destroy(FVM_Stack *);
FVM_API unsigned int fvm_stack_size(const FVM_Stack *);
/* Indices start at 1; negative indices count from the top (-1 is top). */
FVM_API FVM_Result fvm_stack_get(const FVM_Stack *, int, FVM_Value *);
FVM_API FVM_Result fvm_stack_push(FVM_Stack *, const FVM_Value *);
FVM_API FVM_Result fvm_stack_pop(FVM_Stack *, unsigned int);
FVM_API FVM_Result fvm_stack_clear(FVM_Stack *);
FVM_API FVM_Result fvm_stack_register(FVM_Context *, const char *, FVM_StackFunction, void *);
/* Calls a registered stack function with a temporary argument stack.
 * The callback receives exactly argc entries. On success it must leave
 * exactly one result entry; an empty stack means undefined. */
FVM_API FVM_Result fvm_stack_call(FVM_Context *, const char *, unsigned int,
                                  const FVM_Value *, FVM_Value *);
#ifdef __cplusplus
}
#endif
#endif
