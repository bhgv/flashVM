/* FlashVM: scoped AVM1 base/current display targets with roots for suspended caller activations. */
#ifndef FVM_AVM1_TARGET_SCOPE_HPP
#define FVM_AVM1_TARGET_SCOPE_HPP /* Guard for the private display-target activation scope. */
#include "../core/runtime.hpp"
/* [GC:] Publish one activation's target state and restore suspended state on every exit. */
class FVM_AVM1TargetScope
{
    FVM_Context *context_; /* Context whose active display-target state is temporarily replaced. */
    bool active_; /* Whether the suspended caller had an active AVM1 target scope. */
    FVM_Object *base_; /* [GC:] Suspended caller's base clip, also retained in roots_. */
    FVM_Object *target_; /* [GC:] Suspended caller's selected target, also retained in roots_. */
    FVM_GCRootFrame frame_; /* [GC:] Root-chain entry retaining both suspended and new state. */
    fvm_detail::vector<FVM_Value> roots_; /* [GC:] Old base/target and new base/target snapshots. */
    /* Keep root-frame ownership unique. */
    FVM_AVM1TargetScope(const FVM_AVM1TargetScope &);
    /* Keep root-frame ownership unique. */
    FVM_AVM1TargetScope &operator=(const FVM_AVM1TargetScope &);
  public:
    /* [GC:] Prepare roots before publishing inherited block state or a fresh script target. */
    FVM_AVM1TargetScope(FVM_Context *c, FVM_Object *receiver, bool inherit)
        : context_(c), active_(c->avm1_target_active), base_(c->avm1_base_clip),
          target_(c->avm1_target_clip), roots_(4)
    {
        FVM_Object *base = base_, *target = target_;
        if (!inherit || !active_)
        {
            base = receiver && receiver->resource_kind == 40 ? receiver : active_ ? base_ : 0;
            if (!base)
            {
                fvm_detail::map<fvm_detail::string, FVM_Value>::iterator root =
                    c->global->properties.find("_root");
                if (root != c->global->properties.end() && root->second.type == FVM_VALUE_OBJECT &&
                    root->second.object && root->second.object->resource_kind == 40)
                    base = root->second.object;
            }
            target = base;
        }
        FVM_Object *objects[4] = {base_, target_, base, target};
        for (size_t i = 0; i < 4; ++i)
        {
            fvm_value_undefined(&roots_[i]);
            if (objects[i])
            {
                roots_[i].type = FVM_VALUE_OBJECT;
                roots_[i].object = objects[i];
            }
        }
        frame_.previous = c->gc_frames;
        frame_.stack = &roots_;
        frame_.locals = 0;
        frame_.receiver = 0;
        c->gc_frames = &frame_;
        c->avm1_target_active = true;
        c->avm1_base_clip = base;
        c->avm1_target_clip = target;
    }
    /* [GC:] Restore caller metadata before removing the roots that preserve it. */
    ~FVM_AVM1TargetScope()
    {
        context_->avm1_target_active = active_;
        context_->avm1_base_clip = base_;
        context_->avm1_target_clip = target_;
        context_->gc_frames = frame_.previous;
    }
};
#endif
