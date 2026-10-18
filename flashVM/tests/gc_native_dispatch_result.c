#include "flashvm/function.h"
#include "flashvm/callable.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/stack.h"
#include "flashvm/auxlib.h"
#include "flashvm/userdata.h"
#include <assert.h>
#include <string.h>
typedef struct Payload {char bytes[3];unsigned released;} Payload;
static void poison(void *raw){Payload *p=(Payload *)raw;memset(p->bytes,'!',3);++p->released;}
static FVM_Result read(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{
    Payload *p=(Payload *)raw;(void)c;(void)self;(void)n;(void)args;
    return fvm_value_string_bytes(out,p->bytes,3);
}
static FVM_Result replace(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{
    assert(fvm_register_function(c,"read",read,raw)==FVM_OK);
    return read(c,self,n,args,out,raw);
}
static FVM_Result stack_read(FVM_Context *c,FVM_Stack *stack,void *raw)
{
    Payload *p=(Payload *)raw;FVM_Value value;(void)c;
    assert(fvm_stack_clear(stack)==FVM_OK);
    assert(fvm_value_string_bytes(&value,p->bytes,3)==FVM_OK);
    return fvm_stack_push(stack,&value);
}
static Payload *lua_payload;
static int lua_read(FVM_Stack *stack)
{
    FVM_Result r=stack_read(fvm_stack_context(stack),stack,lua_payload);
    return r==FVM_OK?1:(int)r;
}
static FVM_Result stack_replace(FVM_Context *c,FVM_Stack *stack,void *raw)
{
    assert(fvm_stack_register(c,"read",stack_read,raw)==FVM_OK);
    return stack_read(c,stack,raw);
}
static int lua_replace(FVM_Stack *stack)
{
    assert(fvm_stack_register_cfunction(fvm_stack_context(stack),"read",lua_read,0)==FVM_OK);
    return lua_read(stack);
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *owner,*function;
    FVM_Value value;FVM_GCRootScope *scope;Payload payload;int traced,kind;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(traced=0;traced<2;++traced)for(kind=0;kind<5;++kind)
    {
        assert(fvm_context_create(rt,&c)==FVM_OK);
        assert(fvm_gc_string_mode(c,traced?FVM_GC_STRINGS_TRACED:FVM_GC_STRINGS_ARENA)==FVM_OK);
        payload.bytes[0]='A';payload.bytes[1]=0;payload.bytes[2]='B';payload.released=0;
        assert(fvm_object_create(c,&owner)==FVM_OK);assert(fvm_userdata_attach(c,owner,&payload,poison)==FVM_OK);
        if(kind==0)
        {
            assert(fvm_function_create_native(c,read,&payload,owner,&function)==FVM_OK);
            assert(fvm_function_invoke(c,function,0,0,0,&value)==FVM_OK);
        }
        else if(kind==1)
        {
            assert(fvm_register_function_rooted(c,"read",replace,&payload,owner)==FVM_OK);
            assert(fvm_call(c,"read",0,0,&value)==FVM_OK);
        }
        else if(kind==2)
        {
            assert(fvm_stack_register_rooted(c,"read",stack_replace,&payload,owner)==FVM_OK);
            assert(fvm_call(c,"read",0,0,&value)==FVM_OK);
        }
        else if(kind==3)
        {
            assert(fvm_callable_create_rooted(c,stack_read,&payload,owner,&function)==FVM_OK);
            assert(fvm_callable_invoke(c,function,0,0,&value)==FVM_OK);
        }
        else
        {
            lua_payload=&payload;
            assert(fvm_stack_register_cfunction(c,"read",lua_replace,owner)==FVM_OK);
            assert(fvm_call(c,"read",0,0,&value)==FVM_OK);
        }
        assert(value.type==FVM_VALUE_STRING && value.string_size==3 && value.string!=payload.bytes);
        assert(fvm_gc_roots_begin(c,&scope)==FVM_OK);assert(fvm_gc_roots_add(scope,&value)==FVM_OK);
        assert(fvm_gc_collect(c,0)==FVM_OK && payload.released==1);
        assert(!memcmp(payload.bytes,"!!!",3) && !memcmp(value.string,"A\0B",3));
        assert(fvm_gc_roots_end(scope)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
