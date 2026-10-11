#include "../core/runtime.hpp"
#include "flashvm/gc.h"
#include "flashvm/metamethod.h"
#include <climits>

/* Iterative marking avoids recursion overflow on deep/cyclic object graphs. */
static void mark_object(FVM_Object *o, fvm_detail::vector<FVM_Object *> &pending) {
    if (!o || o->gc_marked) return;
    o->gc_marked=true;
    pending.push_back(o);
}
static void mark_graph(FVM_Context *c, size_t newborn_start=(size_t)-1) {
    fvm_detail::vector<FVM_Object *> pending;
    mark_object(c->global,pending);
    fvm_detail::map<fvm_detail::string,FVM_StackEntry>::iterator stack_fn;
    for(stack_fn=c->stack_functions.begin();stack_fn!=c->stack_functions.end();++stack_fn) mark_object(stack_fn->second.root,pending);
    size_t i;
    fvm_detail::map<fvm_detail::string,FVM_NativeEntry>::iterator fn;
    for (fn=c->functions.begin();fn!=c->functions.end();++fn) mark_object(fn->second.root,pending);
    for (fn=c->constructors.begin();fn!=c->constructors.end();++fn) mark_object(fn->second.root,pending);
    for (i=0;i<c->gc_stacks.size();++i)
        for (size_t j=0;j<c->gc_stacks[i]->size();++j) {
            const FVM_Value &v=(*c->gc_stacks[i])[j];
            if (v.type==FVM_VALUE_OBJECT) mark_object(v.object,pending);
        }
    for (i=0;i<c->objects.size();++i)
        if (i>=newborn_start || c->objects[i]->gc_pins) mark_object(c->objects[i],pending);
    for (FVM_GCRootFrame *frame=c->gc_frames;frame;frame=frame->previous) {
        mark_object(frame->receiver,pending);
        if (frame->stack) for(size_t j=0;j<frame->stack->size();++j) {
            const FVM_Value &v=(*frame->stack)[j];
            if(v.type==FVM_VALUE_OBJECT) mark_object(v.object,pending);
        }
        if (frame->locals) {
            fvm_detail::map<fvm_detail::string,FVM_Value>::iterator it;
            for(it=frame->locals->begin();it!=frame->locals->end();++it)
                if(it->second.type==FVM_VALUE_OBJECT) mark_object(it->second.object,pending);
        }
    }
    while (!pending.empty()) {
        FVM_Object *o=pending.back();pending.pop_back();
        if(o->resource_result_valid && o->resource_result.type==FVM_VALUE_OBJECT)mark_object(o->resource_result.object,pending);
        mark_object(o->callable.root,pending);
        for (fvm_detail::map<fvm_detail::string,FVM_AccessorEntry>::iterator accessor=o->accessors.begin();
             accessor!=o->accessors.end();++accessor) mark_object(accessor->second.root,pending);
        mark_object(o->prototype,pending);
        mark_object(o->metatable,pending);
        for(fn=o->methods.begin();fn!=o->methods.end();++fn) mark_object(fn->second.root,pending);
        for(fn=o->metamethods.begin();fn!=o->metamethods.end();++fn) mark_object(fn->second.root,pending);
        fvm_detail::map<fvm_detail::string,FVM_Value>::iterator p;
        for (p=o->properties.begin();p!=o->properties.end();++p)
            if (p->second.type==FVM_VALUE_OBJECT) mark_object(p->second.object,pending);
        fvm_detail::map<unsigned long,FVM_Value>::iterator q;
        for (q=o->indexed.begin();q!=o->indexed.end();++q)
            if (q->second.type==FVM_VALUE_OBJECT) mark_object(q->second.object,pending);
    }
}
extern "C" {
FVM_Result fvm_gc_mode(FVM_Context *c,int mode) {
    if (!c || mode < -100 || mode > FVM_GC_FULL) return FVM_INVALID_ARGUMENT;
    if (mode>100 && mode!=FVM_GC_SUSPENDED && mode!=FVM_GC_FULL) return FVM_INVALID_ARGUMENT;
    if (mode==FVM_GC_FULL) return fvm_gc_collect(c,0);
    c->gc_mode=mode;return FVM_OK;
}
FVM_Result fvm_gc_state(FVM_Context *c,int *mode) {
    if (!c || !mode) return FVM_INVALID_ARGUMENT;
    *mode=c->gc_mode;return FVM_OK;
}
FVM_Result fvm_gc_pin(FVM_Context *c,FVM_Object *o) {
    if (!c || !o || c->gc_releasing || o->owner!=c || o->gc_pins==ULONG_MAX) return FVM_INVALID_ARGUMENT;
    ++o->gc_pins;return FVM_OK;
}
FVM_Result fvm_gc_pin_count(FVM_Context *c,FVM_Object *o,unsigned long *count) {
    if (!c || !o || !count || o->owner!=c) return FVM_INVALID_ARGUMENT;
    *count=o->gc_pins;return FVM_OK;
}
FVM_Result fvm_gc_unpin(FVM_Context *c,FVM_Object *o) {
    if (!c || !o || c->gc_releasing || o->owner!=c || !o->gc_pins) return FVM_INVALID_ARGUMENT;
    --o->gc_pins;return FVM_OK;
}
FVM_Result fvm_gc_object_count(FVM_Context *c,unsigned long *n) {
    if (!c || !n) return FVM_INVALID_ARGUMENT;
    *n=(unsigned long)c->objects.size();return FVM_OK;
}
FVM_Result fvm_gc_collect(FVM_Context *c,unsigned long *reclaimed) {
    if (!c || c->gc_collecting || c->gc_native_depth) return FVM_INVALID_ARGUMENT;
    if (reclaimed) *reclaimed=0;
    c->gc_collecting=true;
    size_t i;
    const size_t original_count=c->objects.size();
    FVM_Result finalizer_result=FVM_OK;
    try {
        for(i=0;i<c->objects.size();++i)c->objects[i]->gc_marked=false;
        mark_graph(c);
        /* Finalizers may resurrect objects by placing them into the global graph.
           Mark again after all finalizers, before deleting anything. */
        for(i=0;i<original_count;++i) {
            FVM_Object *o=c->objects[i];
            if (!o->gc_marked) {
                FVM_Result r=fvm_meta_finalize(c,o);
                if(finalizer_result==FVM_OK && r!=FVM_OK) finalizer_result=r;
            }
        }
        for(i=0;i<c->objects.size();++i)c->objects[i]->gc_marked=false;
        /* Snapshot bounds finalization; newborn graphs survive this pass. */
        mark_graph(c,original_count);
        /* Resource callbacks see a coherent heap: do not delete objects until
           every unreachable resource has been released. */
        for(i=0;i<c->objects.size();++i) if(!c->objects[i]->gc_marked) {
            FVM_Result r=fvm_release_object_resource(c,c->objects[i]);
            if(finalizer_result==FVM_OK && r!=FVM_OK)finalizer_result=r;
        }
        unsigned long freed=0;
        size_t kept=0;
        for(i=0;i<c->objects.size();++i) {
            FVM_Object *o=c->objects[i];
            if(o->gc_marked) { c->objects[kept++]=o; continue; }
            delete o;
            ++freed;
        }
        c->objects.resize(kept);
        if(reclaimed)*reclaimed=freed;
        c->gc_allocations=0;
    } catch(...) {c->gc_collecting=false;return FVM_OUT_OF_MEMORY;}
    c->gc_collecting=false;
    return finalizer_result;
}
FVM_Result fvm_gc_pending_allocations(FVM_Context *c,unsigned long *count) {
    if (!c || !count) return FVM_INVALID_ARGUMENT;
    *count=c->gc_allocations;return FVM_OK;
}
FVM_Result fvm_gc_threshold(FVM_Context *c,unsigned long *threshold) {
    if (!c || !threshold) return FVM_INVALID_ARGUMENT;
    if (c->gc_mode==FVM_GC_SUSPENDED) { *threshold=ULONG_MAX;return FVM_OK; }
    *threshold=(unsigned long)(1000-c->gc_mode*9);return FVM_OK;
}
FVM_Result fvm_gc_stats(FVM_Context *c,FVM_GCStats *out) {
    if (!c || !out || out->struct_size!=sizeof(FVM_GCStats)) return FVM_INVALID_ARGUMENT;
    out->live_objects=(unsigned long)c->objects.size();
    out->pinned_objects=0;
    for(size_t i=0;i<c->objects.size();++i)
        if(c->objects[i]->gc_pins) ++out->pinned_objects;
    out->pending_allocations=c->gc_allocations;
    out->mode=c->gc_mode;
    out->collecting=c->gc_collecting ? 1 : 0;
    out->native_depth=c->gc_native_depth;
    return fvm_gc_threshold(c,&out->threshold);
}
FVM_Result fvm_gc_step(FVM_Context *c,unsigned long *reclaimed) {
    if (!c)return FVM_INVALID_ARGUMENT;
    if(reclaimed)*reclaimed=0;
    if (c->gc_native_depth) return FVM_OK;
    if(c->gc_mode==FVM_GC_SUSPENDED)return FVM_OK;
    /* The allocation threshold is monotonic with the tuning setting. */
    unsigned long threshold=0;
    fvm_gc_threshold(c,&threshold);
    if(c->gc_allocations<threshold)return FVM_OK;
    return fvm_gc_collect(c,reclaimed);
}
}
