#include "flashvm/stack.h"
#include "flashvm/callable.h"
#include "../core/runtime.hpp"
#include <new>
#include <cstring>
struct FVM_Stack {
    FVM_Context *context;
    fvm_detail::vector<FVM_Value> values;
};
static int valid(FVM_Context *c, const FVM_Value *v) {
    return v && v->type>=FVM_VALUE_UNDEFINED && v->type<=FVM_VALUE_OBJECT &&
        (v->type!=FVM_VALUE_OBJECT || (v->object && v->object->owner==c)) &&
        (v->type!=FVM_VALUE_STRING || v->string);
}
extern "C" {
FVM_Result fvm_stack_create(FVM_Context *c,FVM_Stack **out) {
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    *out=new(std::nothrow) FVM_Stack;
    if (!*out) return FVM_OUT_OF_MEMORY;
    (*out)->context=c;
    return FVM_OK;
}
void fvm_stack_destroy(FVM_Stack *s) { delete s; }
unsigned int fvm_stack_size(const FVM_Stack *s) { return s?(unsigned int)s->values.size():0; }
FVM_Result fvm_stack_get(const FVM_Stack *s,int index,FVM_Value *out) {
    long n, pos;
    if (!s || !out || index==0) return FVM_INVALID_ARGUMENT;
    n=(long)s->values.size(); pos=index>0?(long)index-1:n+(long)index;
    if (pos<0 || pos>=n) return FVM_INVALID_ARGUMENT;
    *out=s->values[(size_t)pos]; return FVM_OK;
}
FVM_Result fvm_stack_push(FVM_Stack *s,const FVM_Value *v) {
    if (!s || !valid(s->context,v)) return FVM_INVALID_ARGUMENT;
    try { s->values.push_back(*v); } catch (...) { return FVM_OUT_OF_MEMORY; }
    return FVM_OK;
}
FVM_Result fvm_stack_pop(FVM_Stack *s,unsigned int count) {
    if (!s || count>s->values.size()) return FVM_INVALID_ARGUMENT;
    while(count--) s->values.pop_back();
    return FVM_OK;
}
FVM_Result fvm_stack_clear(FVM_Stack *s) {
    if (!s) return FVM_INVALID_ARGUMENT;
    return fvm_stack_pop(s,(unsigned int)s->values.size());
}
FVM_Result fvm_stack_register(FVM_Context *c,const char *name,FVM_StackFunction cb,void *user) {
    FVM_StackEntry e;
    if (!c || !name || !*name || !cb) return FVM_INVALID_ARGUMENT;
    e.callback=cb; e.userdata=user;
    try { c->stack_functions[name]=e; } catch (...) { return FVM_OUT_OF_MEMORY; }
    return FVM_OK;
}
FVM_Result fvm_stack_call(FVM_Context *c,const char *name,unsigned int argc,const FVM_Value *args,FVM_Value *out) {
    FVM_Stack *s=0; FVM_Result r; unsigned int i;
    if (!c || !name || !out || (argc && !args)) return FVM_INVALID_ARGUMENT;
    fvm_detail::map<fvm_detail::string,FVM_StackEntry>::iterator it=c->stack_functions.find(name);
    if (it==c->stack_functions.end()) return FVM_UNSUPPORTED;
    r=fvm_stack_create(c,&s); if (r!=FVM_OK) return r;
    for(i=0;i<argc;++i) { r=fvm_stack_push(s,&args[i]); if(r!=FVM_OK) break; }
    if(r==FVM_OK) {
        try { r=it->second.callback(c,s,it->second.userdata); }
        catch (...) { r=FVM_ERROR; }
    }
    if(r==FVM_OK) {
        if(s->values.size()>1) r=FVM_ERROR;
        else if(s->values.size()==1) *out=s->values[0];
        else fvm_value_undefined(out);
    }
    fvm_stack_destroy(s); return r;
}
}

extern "C" {
FVM_Result fvm_callable_create(FVM_Context *c, FVM_StackFunction cb,
                               void *user, FVM_Object **out) {
    FVM_Object *obj=0;
    FVM_StackEntry entry;
    FVM_Result r;
    if (!c || !cb || !out) return FVM_INVALID_ARGUMENT;
    *out=0;
    r=fvm_object_create(c,&obj);
    if (r!=FVM_OK) return r;
    entry.callback=cb;
    entry.userdata=user;
    try { c->callable_objects[(unsigned long)obj]=entry; }
    catch (...) { return FVM_OUT_OF_MEMORY; }
    *out=obj;
    return FVM_OK;
}
FVM_Result fvm_callable_is(FVM_Context *c,FVM_Object *obj,int *out) {
    if (!c || !obj || !out || obj->owner!=c) return FVM_INVALID_ARGUMENT;
    *out=c->callable_objects.find((unsigned long)obj)!=c->callable_objects.end();
    return FVM_OK;
}
FVM_Result fvm_callable_invoke(FVM_Context *c,FVM_Object *obj,
                               unsigned int argc,const FVM_Value *args,FVM_Value *out) {
    FVM_Stack *stack=0;
    FVM_Result r;
    unsigned int i;
    if (!c || !obj || !out || obj->owner!=c || (argc && !args))
        return FVM_INVALID_ARGUMENT;
    fvm_detail::map<unsigned long,FVM_StackEntry>::iterator it=c->callable_objects.find((unsigned long)obj);
    if (it==c->callable_objects.end()) return FVM_UNSUPPORTED;
    r=fvm_stack_create(c,&stack);
    if (r!=FVM_OK) return r;
    for(i=0;i<argc;++i) {
        r=fvm_stack_push(stack,&args[i]);
        if (r!=FVM_OK) break;
    }
    if (r==FVM_OK) {
        try { r=it->second.callback(c,stack,it->second.userdata); }
        catch (...) { r=FVM_ERROR; }
    }
    if (r==FVM_OK) {
        if (stack->values.size()>1) r=FVM_ERROR;
        else if (stack->values.size()==1) *out=stack->values[0];
        else fvm_value_undefined(out);
    }
    fvm_stack_destroy(stack);
    return r;
}
}
