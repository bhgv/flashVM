#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include <assert.h>
#include <string.h>
static FVM_Result invalid(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *args,FVM_Value *out,void *raw)
{(void)c;(void)self;(void)n;(void)args;*out=*(FVM_Value *)raw;return FVM_OK;}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c,*other;FVM_Object *foreign,*function,*object,*meta;
    FVM_Value bad,out,owner;FVM_GCStringStats stats;unsigned short bytes[3]={'A',0,'B'};
    int kind,case_id;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_object_create(other,&foreign)==FVM_OK);
    for(kind=0;kind<6;++kind)for(case_id=0;case_id<6;++case_id)
    {
        assert(fvm_context_create(rt,&c)==FVM_OK);fvm_value_undefined(&bad);
        if(case_id==0)memset(&bad.type,0x7f,sizeof(bad.type));
        else if(case_id==1){bad.type=FVM_VALUE_STRING;bad.string=0;}
        else if(case_id==2){bad.type=FVM_VALUE_OBJECT;bad.object=foreign;}
        else if(case_id==3){fvm_value_number(&bad,7);bad.metatable=foreign;}
        else if(case_id==4){assert(fvm_value_string(&bad,"external")==FVM_OK);bad.metatable=foreign;}
        else {assert(fvm_value_string_utf16(c,bytes,3,&bad)==FVM_OK);bad.string_size=4;}
        if(kind==0)
        {
            assert(fvm_function_create_native(c,invalid,&bad,0,&function)==FVM_OK);
            assert(fvm_function_invoke(c,function,0,0,0,&out)==FVM_INVALID_ARGUMENT);
        }
        else if(kind==1)
        {
            assert(fvm_register_function(c,"invalid",invalid,&bad)==FVM_OK);
            assert(fvm_call(c,"invalid",0,0,&out)==FVM_INVALID_ARGUMENT);
        }
        else
        {
            assert(fvm_object_create(c,&object)==FVM_OK);
            if(kind==2)
            {
                assert(fvm_object_define_method_rooted(c,object,"invalid",invalid,&bad,0)==FVM_OK);
                assert(fvm_object_call(c,object,"invalid",0,0,&out)==FVM_INVALID_ARGUMENT);
            }
            else if(kind==3)
            {
                assert(fvm_object_define_accessor_rooted(c,object,"invalid",invalid,0,&bad,0)==FVM_OK);
                assert(fvm_object_get(c,object,"invalid",&out)==FVM_INVALID_ARGUMENT);
            }
            else
            {
                assert(fvm_object_create(c,&meta)==FVM_OK);
                assert(fvm_meta_register(c,meta,kind==4?"__index":"__call",invalid,&bad)==FVM_OK);
                if(kind==4)
                {
                    assert(fvm_table_set_metatable(c,object,meta)==FVM_OK);
                    assert(fvm_object_get(c,object,"missing",&out)==FVM_INVALID_ARGUMENT);
                }
                else
                {
                    fvm_value_number(&owner,1);assert(fvm_value_set_metatable(c,&owner,meta)==FVM_OK);
                    assert(fvm_value_meta_invoke(c,&owner,"__call",0,0,&out)==FVM_INVALID_ARGUMENT);
                }
            }
        }
        assert(fvm_gc_string_stats(c,&stats)==FVM_OK && stats.buffers==(case_id==5?1:0));
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_context_destroy(other);fvm_runtime_destroy(rt);return 0;
}
