/* C89 enumeration snapshots preserve counted names, hiding and nonobject policy. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
static FVM_Result forbidden(FVM_Context *c,FVM_Object *o,unsigned n,const FVM_Value *a,FVM_Value *v,void *p)
{(void)c;(void)o;(void)n;(void)a;(void)v;(void)p;assert(0);return FVM_ERROR;}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;unsigned version;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(version=4;version<=8;++version)
    {
        FVM_Context *c;FVM_Object *o,*parent;FVM_Value v,keys,key;size_t count,i;
        unsigned seen=0; int deleted=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        assert(fvm_get_global(c,"Array",&v)==FVM_OK && v.type==FVM_VALUE_OBJECT);
        assert(fvm_object_get(c,v.object,"prototype",&key)==FVM_OK && key.type==FVM_VALUE_OBJECT);
        assert(fvm_object_define_accessor_rooted(c,key.object,"0",forbidden,forbidden,0,0)==FVM_OK);
        assert(fvm_object_set_property_flags(c,v.object,"prototype",0,FVM_PROPERTY_VALID_FLAGS)==FVM_OK);
        assert(fvm_object_delete(c,v.object,"prototype",&deleted)==FVM_OK && deleted);
        assert(fvm_object_define_accessor_rooted(c,v.object,"prototype",forbidden,0,0,0)==FVM_OK);
        for(i=0;i<5;++i)
        {
            fvm_value_undefined(&v);
            if(i==1)v.type=FVM_VALUE_NULL;
            if(i==2)fvm_value_number(&v,17);
            if(i==3)fvm_value_boolean(&v,1);
            if(i==4)assert(fvm_value_string(&v,"abc")==FVM_OK);
            assert(fvm_exec_enumerate(c,&v,&v)==FVM_OK && v.type==FVM_VALUE_OBJECT);
            assert(fvm_table_indexed_count(c,v.object,&count)==FVM_OK && count==0);
        }
        assert(fvm_object_create(c,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK);
        assert(fvm_object_create(c,&parent)==FVM_OK && fvm_object_set_prototype(c,o,parent)==FVM_OK);
        fvm_value_number(&v,17);
        assert(fvm_object_set(c,parent,"hidden",&v)==FVM_OK && fvm_object_set(c,parent,"inherited",&v)==FVM_OK);
        assert(fvm_object_set(c,o,"hidden",&v)==FVM_OK);
        assert(fvm_object_set_property_flags(c,o,"hidden",FVM_PROPERTY_DONT_ENUM,0)==FVM_OK);
        assert(fvm_object_set_bytes(c,o,"n\0k",3,&v)==FVM_OK);
        assert(fvm_object_define_accessor_rooted(c,o,"accessor",forbidden,forbidden,0,0)==FVM_OK);
        fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
        assert(fvm_exec_enumerate(c,&v,&keys)==FVM_OK && fvm_gc_pin(c,keys.object)==FVM_OK);
        assert(fvm_object_set(c,o,"later",&v)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        assert(fvm_table_indexed_count(c,keys.object,&count)==FVM_OK && count==3);
        for(i=0;i<count;++i)
        {
            assert(fvm_table_get_index(c,keys.object,(unsigned long)i,&key)==FVM_OK && key.type==FVM_VALUE_STRING);
            if(fvm_value_string_size(&key)==3 && !memcmp(key.string,"n\0k",3))seen|=1;
            else if(!strcmp(key.string,"inherited"))seen|=2;
            else if(!strcmp(key.string,"accessor"))seen|=4;
            else assert(0);
        }
        assert(seen==7);fvm_context_destroy(c);
    }
    {
        FVM_Context *c;FVM_Object *klass,*target;FVM_Value input,out;size_t count;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_object_create(c,&klass)==FVM_OK);
        assert(fvm_object_define_accessor_rooted(c,klass,"prototype",forbidden,0,0,0)==FVM_OK);
        assert(fvm_register_constructor_rooted(c,"Array",forbidden,0,klass)==FVM_OK);
        fvm_value_number(&input,17);
        assert(fvm_exec_enumerate(c,&input,&out)==FVM_OK);
        assert(fvm_table_indexed_count(c,out.object,&count)==FVM_OK && count==0);
        assert(fvm_object_create(c,&target)==FVM_OK);
        assert(fvm_object_set(c,target,"member",&input)==FVM_OK);
        input.type=FVM_VALUE_OBJECT;input.object=target;
        assert(fvm_exec_enumerate(c,&input,&out)==FVM_OK);
        assert(fvm_table_indexed_count(c,out.object,&count)==FVM_OK && count==1);
        fvm_context_destroy(c);
    }
    {
        FVM_Context *c;FVM_Value input,out;
        assert(fvm_context_create(rt,&c)==FVM_OK);fvm_value_number(&input,17);fvm_value_number(&out,99);
        assert(fvm_exec_enumerate(c,&input,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_enumerate(0,&input,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
