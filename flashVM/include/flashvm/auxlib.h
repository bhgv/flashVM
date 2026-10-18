/* FlashVM: Public embedding API declarations: auxlib.
 * File: include/flashvm/auxlib.h.
 */

#ifndef FLASHVM_AUXLIB_H
/* Define FLASHVM AUXLIB H constant or compilation switch. */
#define FLASHVM_AUXLIB_H
#include "stack.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Lua-style callback: arguments start at 1, push results and return their
     * count. This AVM1 bridge accepts zero or one result. Negative returns are
     * FVM_Result errors. No longjmp is used across C++ frames. */
    typedef int (*FVM_CFunction)(FVM_Stack *);
    /* Register stack cfunction. */
    FVM_API FVM_Result fvm_stack_register_cfunction(FVM_Context *, const char *, FVM_CFunction,
                                                    FVM_Object *);
    /* [GC:] Register a counted CFunction key and traced capture. */
    FVM_API FVM_Result fvm_stack_register_cfunction_bytes(FVM_Context *, const char *, size_t,
                                                          FVM_CFunction, FVM_Object *);
    /* Copy all string bytes, including NUL and lone surrogates, onto the stack. */
    FVM_API FVM_Result fvm_stack_pushstring_bytes(FVM_Stack *, const char *, size_t);
    /* Query a string view with its full byte count; no terminator is required. */
    FVM_API FVM_Result fvm_stack_checkstring_bytes(FVM_Stack *, int, const char **, size_t *);
    /* Read a field using a complete counted name and push its value. */
    FVM_API FVM_Result fvm_stack_getfield_bytes(FVM_Stack *, int, const char *, size_t);
    /* Assign a counted field; consume the top value only after success. */
    FVM_API FVM_Result fvm_stack_setfield_bytes(FVM_Stack *, int, const char *, size_t);
    /* Read a counted global key and push its value. */
    FVM_API FVM_Result fvm_stack_getglobal_bytes(FVM_Stack *, const char *, size_t);
    /* Assign a counted global key; consume the top value only after success. */
    FVM_API FVM_Result fvm_stack_setglobal_bytes(FVM_Stack *, const char *, size_t);
    /* Return the interpreter context associated with the stack handle. */
    FVM_API FVM_Context *fvm_stack_context(FVM_Stack *);
    /* Implement stack settop for this subsystem. */
    FVM_API FVM_Result fvm_stack_settop(FVM_Stack *, int);
    /* Implement stack pushvalue for this subsystem. */
    FVM_API FVM_Result fvm_stack_pushvalue(FVM_Stack *, int);
    /* Remove stack. */
    FVM_API FVM_Result fvm_stack_remove(FVM_Stack *, int);
    /* Insert stack. */
    FVM_API FVM_Result fvm_stack_insert(FVM_Stack *, int);
    /* Implement stack replace for this subsystem. */
    FVM_API FVM_Result fvm_stack_replace(FVM_Stack *, int);
    /* Implement stack pushnumber for this subsystem. */
    FVM_API FVM_Result fvm_stack_pushnumber(FVM_Stack *, double);
    /* Implement stack pushboolean for this subsystem. */
    FVM_API FVM_Result fvm_stack_pushboolean(FVM_Stack *, int);
    /* Copies a NUL-terminated UTF-8 string into context-owned storage. */
    /* Implement stack pushstring for this subsystem. */
    FVM_API FVM_Result fvm_stack_pushstring(FVM_Stack *, const char *);
    /* Implement stack pushundefined for this subsystem. */
    FVM_API FVM_Result fvm_stack_pushundefined(FVM_Stack *);
    /* Implement stack checknumber for this subsystem. */
    FVM_API FVM_Result fvm_stack_checknumber(FVM_Stack *, int, double *);
    /* Implement stack checkstring for this subsystem. */
    FVM_API FVM_Result fvm_stack_checkstring(FVM_Stack *, int, const char **);
    /* Implement stack newtable for this subsystem. */
    FVM_API FVM_Result fvm_stack_newtable(FVM_Stack *);
    /* Implement stack getfield for this subsystem. */
    FVM_API FVM_Result fvm_stack_getfield(FVM_Stack *, int, const char *);
    /* setfield consumes the top value only on success. */
    /* Implement stack setfield for this subsystem. */
    FVM_API FVM_Result fvm_stack_setfield(FVM_Stack *, int, const char *);
    /* Implement stack getglobal for this subsystem. */
    FVM_API FVM_Result fvm_stack_getglobal(FVM_Stack *, const char *);
    /* Implement stack setglobal for this subsystem. */
    FVM_API FVM_Result fvm_stack_setglobal(FVM_Stack *, const char *);
#ifdef __cplusplus
}
#endif
#endif
