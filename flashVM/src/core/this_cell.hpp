/* FlashVM: allocation-free typed this cells for interpreted and generated activations. */
#ifndef FVM_CORE_THIS_CELL_HPP
#define FVM_CORE_THIS_CELL_HPP /* Private activation cell scope guard. */
#include "runtime.hpp"


/* [GC:] Publish the typed root value and synchronize legacy object/Undefined views. */
inline void fvm_this_cell_update(FVM_Context *c, const FVM_Value &value)
{
    c->script_this_cell->value = value;
    c->script_this = value.type == FVM_VALUE_OBJECT ? value.object : 0;
    c->script_this_undefined = c->swf_loaded && value.type == FVM_VALUE_UNDEFINED;
}


/* Select an explicit activation value, inherited cell, or embedding object fallback. */
inline FVM_Value fvm_this_cell_initial(FVM_Context *c, const FVM_Value *prepared, FVM_Object *fallback)
{
    if (prepared)
    {
        return *prepared;
    }
    if (c->script_this_cell)
    {
        return c->script_this_cell->value;
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    if (!c->script_this_undefined && fallback)
    {
        value.type = FVM_VALUE_OBJECT;
        value.object = fallback;
    }
    return value;
}


/* [GC:] Link a stack-owned cell; the collector traces every suspended predecessor. */
class FVM_ThisCellScope
{
    bool enabled_;                /* Whether this scope owns a new cell rather than shared block state. */
    FVM_Context *context_;        /* Context whose cell stack is scoped. */
    FVM_ThisCell cell_;           /* [GC:] Typed value and suspended predecessor. */
    FVM_Object *previous_object_; /* [GC:] Legacy object view restored when no predecessor exists. */
    bool previous_undefined_;     /* Legacy Undefined view restored on all exits. */
    FVM_ThisCellScope(const FVM_ThisCellScope &);            /* Stack links cannot be copied. */
    FVM_ThisCellScope &operator=(const FVM_ThisCellScope &); /* Stack links cannot be assigned. */

  public:


    /* [GC:] Publish a fully initialized cell without allocating or invoking user code. */
    FVM_ThisCellScope(FVM_Context *c, const FVM_Value &value, bool enabled = true)
        : enabled_(enabled), context_(c), previous_object_(c->script_this),
          previous_undefined_(c->script_this_undefined)
    {
        if (!enabled_)
        {
            return;
        }
        cell_.previous = c->script_this_cell;
        cell_.value = value;
        c->script_this_cell = &cell_;
        fvm_this_cell_update(c, value);
    }


    /* [GC:] Restore the suspended cell and object view on success, throw and error. */
    ~FVM_ThisCellScope()
    {
        if (!enabled_)
        {
            return;
        }
        context_->script_this_cell = cell_.previous;
        context_->script_this = previous_object_;
        context_->script_this_undefined = previous_undefined_;
    }
};
#endif
