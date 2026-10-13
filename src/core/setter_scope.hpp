/* FlashVM: allocation-free accessor-owner metadata and roots for nested setter invocation. */
#ifndef FVM_CORE_SETTER_SCOPE_HPP
#define FVM_CORE_SETTER_SCOPE_HPP /* Guard for the private C++98 setter scope. */
#include "object_root_scope.hpp"
/* [GC:] Keep receiver, lookup owner and descriptor/capture alive during a setter callback. */
class FVM_SetterScope
{
    FVM_Context *context_; /* Context whose setter lookup metadata is scoped. */
    FVM_Object *old_owner_; /* [GC:] Prior owner restored with its enclosing active roots. */
    FVM_ObjectRootScope owner_roots_; /* [GC:] Root the object that supplied the accessor. */
    FVM_ObjectRootScope receiver_roots_; /* [GC:] Retain the original receiver through callback mutation. */
    FVM_ObjectRootScope capture_roots_; /* [GC:] Retain a detached accessor's descriptor/capture graph. */
    /* A linked activation frame cannot be copied. */
    FVM_SetterScope(const FVM_SetterScope &);
    /* Assignment would lose the previous activation chain. */
    FVM_SetterScope &operator=(const FVM_SetterScope &);
  public:
    /* [GC:] Publish lookup ownership without changing constructor/method invocation identity. */
    FVM_SetterScope(FVM_Context *c, FVM_Object *owner, FVM_Object *receiver, FVM_Object *capture)
        : context_(c), old_owner_(c->calling_setter_owner), owner_roots_(c, owner),
          receiver_roots_(c, receiver), capture_roots_(c, capture)
    { c->calling_setter_owner = owner; }
    /* [GC:] Restore metadata before root members unwind on every callback return path. */
    ~FVM_SetterScope() { context_->calling_setter_owner = old_owner_; }
};
#endif
