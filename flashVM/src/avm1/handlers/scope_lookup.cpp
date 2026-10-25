/* FlashVM: compatible lexical lookup facade over class-aware binding resolution. */
#include "support.hpp"


/* Preserve the established C interface while ignoring optional binding-class metadata. */
extern "C" FVM_Result fvm_exec_scope_lookup(
    FVM_Context *c, FVM_Object *scope, const FVM_Value *name, FVM_Value *out, FVM_Object **owner, int *found)
{
    int with_scope = 0;
    return fvm_exec_scope_resolve(c, scope, name, out, owner, found, &with_scope);
}
