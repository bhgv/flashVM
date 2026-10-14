/* FlashVM: scoped publication of interpreter lexical nodes, independent from bytecode layout. */
#ifndef FVM_AVM1_LEXICAL_SCOPE_HPP
#define FVM_AVM1_LEXICAL_SCOPE_HPP /* Private lexical-scope guard. */
#include "../core/runtime.hpp"
/* [GC:] Publish at most one managed scope and restore the caller on every exit. */
class FVM_AVM1LexicalScope
{
    FVM_Context *context_; /* [GC:] Context retaining the published lexical node. */
    bool active_;          /* Whether publication succeeded and needs a matching pop. */
    /* [GC:] Scope ownership cannot be copied. */
    FVM_AVM1LexicalScope(const FVM_AVM1LexicalScope &);
    /* [GC:] Scope ownership cannot be assigned. */
    FVM_AVM1LexicalScope &operator=(const FVM_AVM1LexicalScope &);

  public:
    /* [GC:] Start without changing the lexical chain. */
    explicit FVM_AVM1LexicalScope(FVM_Context *c) : context_(c), active_(false)
    {
    }
    /* [GC:] Publish once; a failed vector allocation leaves the guard inactive. */
    void publish(FVM_Object *scope)
    {
        context_->lexical_scopes.push_back(scope);
        active_ = true;
    }
    /* [GC:] Restore the enclosing scope on success, exception or allocation failure. */
    ~FVM_AVM1LexicalScope()
    {
        if (active_)
            context_->lexical_scopes.pop_back();
    }
};
#endif
