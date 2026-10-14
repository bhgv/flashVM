/* C89 traversal roots objects across script getters and never reads the final member. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <assert.h>
static unsigned calls;static int failure,detach,in_host;
static FVM_Result getter(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{ ++calls;if(detach){int removed=0;assert(fvm_object_delete(c,frame->receiver,(const char *)raw,&removed)==FVM_OK && removed);}assert(fvm_gc_collect(c,0)==(in_host?FVM_INVALID_ARGUMENT:FVM_OK));if(failure){FVM_Value e;fvm_value_number(&e,77);return fvm_exception_raise(c,&e);}fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=frame->capture;return FVM_OK; }
static FVM_Result host(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{ FVM_Value base;FVM_Result r;(void)raw;assert(n==1);fvm_value_undefined(&base);base.type=FVM_VALUE_OBJECT;base.object=self;in_host=1;r=fvm_exec_delete_path(c,&base,args,out);in_host=0;return r; }
static FVM_Result native_getter(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{ (void)self;(void)n;(void)args;assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=(FVM_Object *)raw;return FVM_OK; }
static FVM_Result forbidden(FVM_Context *c,FVM_Object *o,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{ (void)c;(void)o;(void)n;(void)a;(void)out;(void)raw;assert(0);return FVM_ERROR; }
static void install(FVM_Context *c,FVM_Object *o,const char *name,FVM_Object *child)
{
    const unsigned char code[]={0};FVM_Object *fn;FVM_Value args[3],out;
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,getter,(void *)name,child)==FVM_OK);
    fvm_value_string(args,name);fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=fn;fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;
    assert(fvm_object_call(c,o,"addProperty",3,args,&out)==FVM_OK && out.boolean);
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *o,*child,*leaf;FVM_Value base,path,value,out;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK && fvm_construct(c,"Object",0,0,&child)==FVM_OK && fvm_construct(c,"Object",0,0,&leaf)==FVM_OK);
    install(c,o,"child",child);install(c,child,"leaf",leaf);
    assert(fvm_object_define_accessor_rooted(c,leaf,"final",forbidden,0,0,0)==FVM_OK);
    fvm_value_undefined(&base);base.type=FVM_VALUE_OBJECT;base.object=o;fvm_value_string(&path,"child:leaf.final");
    assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_OK && out.boolean && calls==2);
    fvm_value_number(&value,17);assert(fvm_object_set_bytes(c,leaf,"a\0b/c",5,&value)==FVM_OK);
    fvm_value_string_bytes(&path,"child.leaf.a\0b/c",16);
    assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_OK && out.boolean);
    fvm_value_string(&path,"child.leaf.victim");assert(fvm_object_set(c,leaf,"victim",&value)==FVM_OK);failure=1;fvm_value_number(&out,99);
    assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);
    assert(fvm_object_get(c,leaf,"victim",&out)==FVM_OK && out.number==17);failure=0;
    fvm_value_string(&path,"child/leaf.victim");fvm_value_number(&out,99);
    failure=1;assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);failure=0;
    assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_OK && out.boolean);
    assert(fvm_object_set(c,leaf,"victim",&value)==FVM_OK);
    {
        char long_path[1025];unsigned i;
        for(i=0;i<1024;++i)long_path[i]='.';
        long_path[1024]=0;
        fvm_value_string(&path,long_path);fvm_value_number(&out,99);
        assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_ERROR && out.number==99);
    }
    {
        FVM_Object *fn;int own;
        assert(fvm_function_create_native(c,host,0,0,&fn)==FVM_OK);
        fvm_value_string(&path,"child.leaf.victim");assert(fvm_function_invoke(c,fn,o,1,&path,&out)==FVM_OK && out.boolean);
        assert(fvm_object_set(c,leaf,"detached",&value)==FVM_OK);detach=1;
        fvm_value_string(&path,"child.leaf.detached");assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_OK && out.boolean);
        assert(fvm_object_has_own(c,o,"child",&own)==FVM_OK && !own);
    }
    assert(fvm_object_define_accessor_rooted(c,o,"native",native_getter,0,leaf,leaf)==FVM_OK);
    assert(fvm_object_set(c,leaf,"victim",&value)==FVM_OK);fvm_value_string(&path,"native.victim");
    assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_OK && out.boolean);
    fvm_value_number(&base,3);fvm_value_string(&path,"member");assert(fvm_exec_delete_path(c,&base,&path,&out)==FVM_OK && !out.boolean);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
