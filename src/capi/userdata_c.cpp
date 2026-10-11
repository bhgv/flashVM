#include "../core/runtime.hpp"
#include "flashvm/userdata.h"
extern "C" FVM_Result fvm_userdata_attach(FVM_Context *c,FVM_Object *o,void *data,FVM_UserdataRelease release) {
    if(!c || !o || o->owner!=c || !release || o->release_resource || c->destroying || c->gc_releasing) return FVM_INVALID_ARGUMENT;
    o->resource_kind=6;o->resource=data;o->release_resource=release;return FVM_OK;
}
extern "C" FVM_Result fvm_userdata_get(FVM_Context *c,FVM_Object *o,void **out) {
    if(!c || !o || o->owner!=c || !out || o->resource_kind!=6) return FVM_INVALID_ARGUMENT;
    *out=o->resource;return FVM_OK;
}
FVM_Result fvm_release_object_resource(FVM_Context *c,FVM_Object *o) {
    if(!o->release_resource) return FVM_OK;
    void (*release)(void *)=o->release_resource;void *data=o->resource;
    o->release_resource=0;o->resource=0;
    bool old=c->gc_releasing;c->gc_releasing=true;
    FVM_Result r=FVM_OK;
    try {release(data);}catch(...){r=FVM_ERROR;}
    c->gc_releasing=old;return r;
}
