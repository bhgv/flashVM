/* FlashVM: rooted accessor-owner metadata and an immutable key for nested setter invocation. */
#ifndef FVM_CORE_SETTER_SCOPE_HPP
#define FVM_CORE_SETTER_SCOPE_HPP /* Guard for the private C++98 setter scope. */
#include "object_root_scope.hpp"


/* [GC:] Keep receiver, lookup owner and descriptor/capture alive during a setter callback. */
class FVM_SetterScope
{
    FVM_Context *context_;               /* Context whose setter lookup metadata is scoped. */
    FVM_Object *old_owner_;              /* [GC:] Prior owner restored with its enclosing active roots. */
    const fvm_detail::string *old_name_; /* Previous scope's borrowed key restored on exit. */
    FVM_ObjectRootScope owner_roots_;    /* [GC:] Root the object that supplied the accessor. */
    FVM_ObjectRootScope receiver_roots_; /* [GC:] Retain the original receiver through callback mutation. */
    FVM_ObjectRootScope capture_roots_;  /* [GC:] Retain a detached accessor's descriptor/capture graph. */
    fvm_detail::string name_;            /* Owned key stays valid when the callback deletes or renames a property. */
    /* A linked activation frame cannot be copied. */
    FVM_SetterScope(const FVM_SetterScope &);
    /* Assignment would lose the previous activation chain. */
    FVM_SetterScope &operator=(const FVM_SetterScope &);

  public:


    /* [GC:] Publish lookup ownership without changing constructor/method invocation identity. */
    FVM_SetterScope(
        FVM_Context *c, FVM_Object *owner, FVM_Object *receiver, FVM_Object *capture, const fvm_detail::string &name)
        : context_(c), old_owner_(c->calling_setter_owner), old_name_(c->calling_setter_name), owner_roots_(c, owner),
          receiver_roots_(c, receiver), capture_roots_(c, capture), name_(name)
    { /* Copying the key can fail before changing metadata; member roots unwind automatically. */
        c->calling_setter_owner = owner;
        c->calling_setter_name = &name_;
    }


    /* [GC:] Restore metadata before root members unwind on every callback return path. */
    ~FVM_SetterScope()
    {
        context_->calling_setter_owner = old_owner_;
        context_->calling_setter_name = old_name_;
    }
};
#endif
