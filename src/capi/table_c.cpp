#include "../core/runtime.hpp"
#include "flashvm/table.h"
#include "flashvm/debug.h"
#include <new>
#include <cstring>
static int valid(FVM_Context *c,FVM_Object *o) { return c && o && o->owner==c; }
static FVM_Result store(FVM_Context *c,const FVM_Value *v,FVM_Value *out) {
    if (!v || v->type<FVM_VALUE_UNDEFINED || v->type>FVM_VALUE_OBJECT ||
        (v->type==FVM_VALUE_OBJECT && (!v->object || v->object->owner!=c)) ||
        (v->type==FVM_VALUE_STRING && !v->string)) return FVM_INVALID_ARGUMENT;
    *out=*v;
    if (v->type==FVM_VALUE_STRING) {
        size_t n=std::strlen(v->string);
        char *p=new(std::nothrow) char[n+1];
        if (!p) return FVM_OUT_OF_MEMORY;
        std::memcpy(p,v->string,n+1);
        try { c->strings.push_back(p); } catch (...) { delete [] p; return FVM_OUT_OF_MEMORY; }
        out->string=p;
    }
    return FVM_OK;
}
extern "C" {
FVM_Result fvm_table_set_index(FVM_Context *c,FVM_Object *o,unsigned long key,const FVM_Value *v) {
    FVM_Value x; FVM_Result r;
    if (!valid(c,o)) return FVM_INVALID_ARGUMENT;
    r=store(c,v,&x); if (r!=FVM_OK) return r;
    try { o->indexed[key]=x; } catch (...) { return FVM_OUT_OF_MEMORY; }
    return FVM_OK;
}
FVM_Result fvm_table_get_index(FVM_Context *c,FVM_Object *o,unsigned long key,FVM_Value *out) {
    if (!valid(c,o) || !out) return FVM_INVALID_ARGUMENT;
    for (unsigned int depth=0;o && depth<1024;++depth) {
        fvm_detail::map<unsigned long,FVM_Value>::const_iterator it=o->indexed.find(key);
        if (it!=o->indexed.end()) { *out=it->second; return FVM_OK; }
        o=o->metatable;
    }
    fvm_value_undefined(out); return FVM_OK;
}
FVM_Result fvm_table_set_metatable(FVM_Context *c,FVM_Object *o,FVM_Object *meta) {
    if (!valid(c,o) || (meta && !valid(c,meta))) return FVM_INVALID_ARGUMENT;
    FVM_Object *cursor=meta;unsigned int depth=0;
    while(cursor && depth++<1024) { if(cursor==o)return FVM_INVALID_ARGUMENT;cursor=cursor->metatable; }
    if(cursor)return FVM_INVALID_ARGUMENT;
    o->metatable=meta;return FVM_OK;
}
FVM_Result fvm_table_get_metatable(FVM_Context *c,FVM_Object *o,FVM_Object **out) {
    if (!valid(c,o) || !out) return FVM_INVALID_ARGUMENT;
    *out=o->metatable; return FVM_OK;
}
FVM_Result fvm_table_named_count(FVM_Context *c,FVM_Object *o,size_t *out) {
    if (!valid(c,o) || !out) return FVM_INVALID_ARGUMENT;
    *out=o->properties.size(); return FVM_OK;
}
FVM_Result fvm_table_indexed_count(FVM_Context *c,FVM_Object *o,size_t *out) {
    if (!valid(c,o) || !out) return FVM_INVALID_ARGUMENT;
    *out=o->indexed.size(); return FVM_OK;
}
FVM_Result fvm_table_named_at(FVM_Context *c,FVM_Object *o,size_t index,const char **name,FVM_Value *out) {
    if (!valid(c,o) || !name || !out || index>=o->properties.size()) return FVM_INVALID_ARGUMENT;
    fvm_detail::map<std::string,FVM_Value>::const_iterator it=o->properties.begin();
    while(index--) ++it;
    *name=it->first.c_str(); *out=it->second; return FVM_OK;
}
FVM_Result fvm_table_indexed_at(FVM_Context *c,FVM_Object *o,size_t index,unsigned long *key,FVM_Value *out) {
    if (!valid(c,o) || !key || !out || index>=o->indexed.size()) return FVM_INVALID_ARGUMENT;
    fvm_detail::map<unsigned long,FVM_Value>::const_iterator it=o->indexed.begin();
    while(index--) ++it;
    *key=it->first; *out=it->second; return FVM_OK;
}
FVM_Result fvm_debug_object_count(FVM_Context *c,size_t *out) {
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    *out=c->objects.size(); return FVM_OK;
}
FVM_Result fvm_debug_object_at(FVM_Context *c,size_t i,FVM_Object **out) {
    if (!c || !out || i>=c->objects.size()) return FVM_INVALID_ARGUMENT;
    *out=c->objects[i]; return FVM_OK;
}
FVM_Result fvm_debug_native_function_count(FVM_Context *c,size_t *out) {
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    *out=c->functions.size(); return FVM_OK;
}
FVM_Result fvm_debug_native_function_at(FVM_Context *c,size_t i,const char **out) {
    if (!c || !out || i>=c->functions.size()) return FVM_INVALID_ARGUMENT;
    fvm_detail::map<std::string,FVM_NativeEntry>::const_iterator it=c->functions.begin();
    while(i--) ++it;
    *out=it->first.c_str(); return FVM_OK;
}
FVM_Result fvm_debug_script_function_count(FVM_Context *c,size_t *out) {
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    *out=c->script_functions.size(); return FVM_OK;
}
FVM_Result fvm_debug_script_function_at(FVM_Context *c,size_t i,const char **out) {
    if (!c || !out || i>=c->script_functions.size()) return FVM_INVALID_ARGUMENT;
    fvm_detail::map<std::string,FVM_ScriptFunction>::const_iterator it=c->script_functions.begin();
    while(i--) ++it;
    *out=it->first.c_str(); return FVM_OK;
}
}
