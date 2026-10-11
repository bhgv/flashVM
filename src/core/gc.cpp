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
static void mark_graph(FVM_Context *c) {
    fvm_detail::vector<FVM_Object *> pending;
    mark_object(c->global,pending);
    size_t i;
    for (i=0;i<c->objects.size();++i)
        if (c->objects[i]->gc_pins) mark_object(c->objects[i],pending);
    while (!pending.empty()) {
        FVM_Object *o=pending.back();pending.pop_back();
        mark_object(o->prototype,pending);
        mark_object(o->metatable,pending);
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
    if (!c || !o || o->owner!=c || o->gc_pins==ULONG_MAX) return FVM_INVALID_ARGUMENT;
    ++o->gc_pins;return FVM_OK;
}
FVM_Result fvm_gc_unpin(FVM_Context *c,FVM_Object *o) {
    if (!c || !o || o->owner!=c || !o->gc_pins) return FVM_INVALID_ARGUMENT;
    --o->gc_pins;return FVM_OK;
}
FVM_Result fvm_gc_object_count(FVM_Context *c,unsigned long *n) {
    if (!c || !n) return FVM_INVALID_ARGUMENT;
    *n=(unsigned long)c->objects.size();return FVM_OK;
}
FVM_Result fvm_gc_collect(FVM_Context *c,unsigned long *reclaimed) {
    if (!c || c->gc_collecting) return FVM_INVALID_ARGUMENT;
    if (reclaimed) *reclaimed=0;
    c->gc_collecting=true;
    size_t i;
    try {
        for(i=0;i<c->objects.size();++i)c->objects[i]->gc_marked=false;
        mark_graph(c);
        /* Finalizers may resurrect objects by placing them into the global graph.
           Mark again after all finalizers, before deleting anything. */
        for(i=0;i<c->objects.size();++i) {
            FVM_Object *o=c->objects[i];
            if (!o->gc_marked) fvm_meta_finalize(c,o);
        }
        for(i=0;i<c->objects.size();++i)c->objects[i]->gc_marked=false;
        mark_graph(c);
        unsigned long freed=0;
        i=0;
        while(i<c->objects.size()) {
            FVM_Object *o=c->objects[i];
            if(o->gc_marked) {++i;continue;}
            delete o;
            c->objects.erase(c->objects.begin()+i);
            ++freed;
        }
        if(reclaimed)*reclaimed=freed;
        c->gc_allocations=0;
    } catch(...) {c->gc_collecting=false;return FVM_OUT_OF_MEMORY;}
    c->gc_collecting=false;
    return FVM_OK;
}
FVM_Result fvm_gc_threshold(FVM_Context *c,unsigned long *threshold) {
    if (!c || !threshold) return FVM_INVALID_ARGUMENT;
    if (c->gc_mode==FVM_GC_SUSPENDED) { *threshold=ULONG_MAX;return FVM_OK; }
    *threshold=(unsigned long)(1000-c->gc_mode*9);return FVM_OK;
}
FVM_Result fvm_gc_step(FVM_Context *c,unsigned long *reclaimed) {
    if (!c)return FVM_INVALID_ARGUMENT;
    if(reclaimed)*reclaimed=0;
    if(c->gc_mode==FVM_GC_SUSPENDED)return FVM_OK;
    /* The allocation threshold is monotonic with the tuning setting. */
    unsigned long threshold=0;
    fvm_gc_threshold(c,&threshold);
    if(c->gc_allocations<threshold)return FVM_OK;
    return fvm_gc_collect(c,reclaimed);
}
}
