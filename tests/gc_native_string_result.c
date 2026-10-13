#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/userdata.h"
#include <assert.h>
#include <string.h>
static unsigned released;
static void poison(void *p){memset(p,'!',3);++released;}
static FVM_Result read(FVM_Context *c,FVM_Object *self,unsigned int n,const FVM_Value *a,FVM_Value *out,void *u)
{
    (void)c;(void)self;(void)n;(void)a;return fvm_value_string_bytes(out,(const char *)u,3);
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *object,*meta;
    FVM_Value value;FVM_GCRootScope *scope;FVM_GCStringStats stats;
    unsigned i;char bytes[3]={'A',0,'B'};
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);
    for(i=0;i<3;++i)
    {
        bytes[0]='A';bytes[1]=0;bytes[2]='B';
        assert(fvm_object_create(c,&object)==FVM_OK);assert(fvm_userdata_attach(c,object,bytes,poison)==FVM_OK);
        assert(fvm_gc_roots_begin(c,&scope)==FVM_OK);
        if(i==0)
        {
            assert(fvm_object_define_accessor_rooted(c,object,"text",read,0,bytes,0)==FVM_OK);
            assert(fvm_object_get(c,object,"text",&value)==FVM_OK);
        }
        else if(i==1)
        {
            assert(fvm_object_define_method_rooted(c,object,"read",read,bytes,0)==FVM_OK);
            assert(fvm_object_call(c,object,"read",0,0,&value)==FVM_OK);
        }
        else
        {
            assert(fvm_object_create(c,&meta)==FVM_OK);
            assert(fvm_meta_register(c,meta,"__index",read,bytes)==FVM_OK);
            assert(fvm_table_set_metatable(c,object,meta)==FVM_OK);
            assert(fvm_object_get(c,object,"missing",&value)==FVM_OK);
        }
        assert(value.type==FVM_VALUE_STRING && value.string_size==3 && value.string!=bytes);
        assert(fvm_gc_roots_add(scope,&value)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
        assert(released==i+1 && bytes[0]=='!' && !memcmp(value.string,"A\0B",3));
        assert(fvm_gc_roots_end(scope)==FVM_OK);
    }
    assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==3 && stats.payload_bytes==9 && stats.allocated_bytes==12);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
