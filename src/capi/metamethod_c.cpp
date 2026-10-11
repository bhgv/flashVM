#include "../core/native_call_guard.hpp"
#include "../core/runtime.hpp"
#include "flashvm/metamethod.h"
#include "flashvm/avm1.h"
#include <cstring>
extern "C" {
FVM_Result fvm_meta_register(FVM_Context *c,FVM_Object *m,const char *name,FVM_NativeFunction cb,void *user) {
    if (!c || !m || m->owner!=c || !name || !cb) return FVM_INVALID_ARGUMENT;
    if (std::strcmp(name,"__index") && std::strcmp(name,"__newindex") &&
        std::strcmp(name,"__iterate") && std::strcmp(name,"__gc") && std::strcmp(name,"__call")) return FVM_UNSUPPORTED;
    FVM_NativeEntry entry;entry.callback=cb;entry.userdata=user;
    try {m->metamethods[name]=entry;}catch(...){return FVM_OUT_OF_MEMORY;}
    return FVM_OK;
}
FVM_Result fvm_meta_finalize(FVM_Context *c,FVM_Object *o) {
    if (!c || !o || o->owner!=c) return FVM_INVALID_ARGUMENT;
    if (o->finalized) return FVM_OK;
    o->finalized=true;
    FVM_Object *m=o->metatable;
    if (!m) return FVM_OK;
    fvm_detail::map<fvm_detail::string,FVM_NativeEntry>::iterator it=m->metamethods.find("__gc");
    fvm_detail::map<fvm_detail::string,FVM_Value>::iterator script=m->properties.find("__gc");
    if (it==m->metamethods.end() && (script==m->properties.end() || script->second.type!=FVM_VALUE_STRING)) return FVM_OK;
    FVM_Value args[3],result;
    fvm_value_undefined(&args[0]);args[0].type=FVM_VALUE_OBJECT;args[0].object=o;
    args[1]=args[0];args[1].object=m;args[2]=args[0];
    fvm_value_undefined(&result);
    if (it==m->metamethods.end()) return fvm_avm1_call(c,script->second.string,o,3,args,&result);
    FVM_NativeEntry entry=it->second;
    try {FVM_NativeCallGuard native_guard(c); return entry.callback(c,o,3,args,&result,entry.userdata);}
    catch(...){return FVM_ERROR;}
}
FVM_Result fvm_meta_iterate(FVM_Context *c,FVM_Object *o,unsigned long index,FVM_Value *key,FVM_Value *value) {
    if (!c || !o || o->owner!=c || !key || !value) return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(key);fvm_value_undefined(value);
    FVM_Object *m=o->metatable;
    if (m) {
        fvm_detail::map<fvm_detail::string,FVM_NativeEntry>::iterator it=m->metamethods.find("__iterate");
        if (it!=m->metamethods.end()) {
            FVM_Value args[3];
            fvm_value_undefined(&args[0]);args[0].type=FVM_VALUE_OBJECT;args[0].object=o;
            args[1]=args[0];args[1].object=m;
            fvm_value_number(&args[2],(double)index);
            try {FVM_NativeCallGuard native_guard(c); return it->second.callback(c,o,3,args,value,it->second.userdata);}
            catch(...){return FVM_ERROR;}
        }
    }
    /* Default iteration: indexed entries followed by named entries. */
    if (index<o->indexed.size()) {
        fvm_detail::map<unsigned long,FVM_Value>::iterator it=o->indexed.begin();
        while(index--)++it;
        fvm_value_number(key,(double)it->first);*value=it->second;return FVM_OK;
    }
    index-=(unsigned long)o->indexed.size();
    if (index<o->properties.size()) {
        fvm_detail::map<fvm_detail::string,FVM_Value>::iterator it=o->properties.begin();
        while(index--)++it;
        fvm_value_string(key,it->first.c_str());*value=it->second;return FVM_OK;
    }
    return FVM_OK;
}
}
