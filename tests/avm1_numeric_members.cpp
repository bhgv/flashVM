/* Standard AVM1 objects use one property namespace for numeric and string member names. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/channel.h"
#include "flashvm/transfer.h"
static FVM_Object *expected;
static unsigned gets,sets;
static double written;
static FVM_Result getter(FVM_Context *,FVM_Object *self,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ assert(self==expected);++gets;fvm_value_number(out,55);return FVM_OK; }
static FVM_Result setter(FVM_Context *,FVM_Object *self,unsigned n,const FVM_Value *a,FVM_Value *out,void *)
{ assert(self==expected && n==1);++sets;written=a[0].number;fvm_value_undefined(out);return FVM_OK; }
static FVM_Value read(FVM_Context *c,FVM_Object *o,double index)
{
    object(c,"owner",o);FVM_Value value;fvm_value_number(&value,index);assert(fvm_set_global(c,"member",&value)==FVM_OK);
    Bytes b;global(b,"owner");global(b,"member");b.push_back(0x4e);return run(c,b);
}
static void write(FVM_Context *c,FVM_Object *o,double index,double number)
{
    object(c,"owner",o);FVM_Value value;fvm_value_number(&value,index);assert(fvm_set_global(c,"member",&value)==FVM_OK);fvm_value_number(&value,number);assert(fvm_set_global(c,"assigned",&value)==FVM_OK);
    Bytes b;global(b,"owner");global(b,"member");global(b,"assigned");b.push_back(0x4f);b.push_back(0);assert(fvm_avm1_execute(c,b.begin(),b.size())==FVM_OK);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_standard_install(other)==FVM_OK);
    FVM_Object *o=0,*proto=0;assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&proto)==FVM_OK);assert(fvm_object_set_prototype(c,o,proto)==FVM_OK);
    FVM_Value value;fvm_value_number(&value,42);assert(fvm_object_set(c,proto,"7",&value)==FVM_OK);
    assert(read(c,o,7).type==FVM_VALUE_NUMBER && read(c,o,7).number==42);
    write(c,o,7,43);assert(fvm_object_get(c,o,"7",&value)==FVM_OK && value.number==43);
    int own=0;assert(fvm_object_has_own(c,o,"7",&own)==FVM_OK && own);
    assert(fvm_object_set_property_flags(c,o,"7",FVM_PROPERTY_READ_ONLY,0)==FVM_OK);write(c,o,7,44);assert(read(c,o,7).number==43);
    expected=o;assert(fvm_object_define_accessor_rooted(c,proto,"8",getter,setter,0,0)==FVM_OK);assert(read(c,o,8).number==55 && gets==1);write(c,o,8,56);assert(sets==1 && written==56);
    write(c,o,-0.0,9);assert(fvm_object_get(c,o,"0",&value)==FVM_OK && value.number==9);
    write(c,o,4294967295.0,10);assert(fvm_object_get(c,o,"4294967295",&value)==FVM_OK && value.number==10);
    fvm_value_number(&value,12);assert(fvm_object_set_bytes(c,o,"7\0x",3,&value)==FVM_OK);assert(read(c,o,7).number==43);
    /* The same named properties survive graph transport into another interpreter. */
    FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=o;
    /* Native test-only accessors have no registered transport codec; omit their prototype first. */
    assert(fvm_object_set_prototype(c,o,0)==FVM_OK);assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK);assert(fvm_channel_receive_value(other,channel,&value,0)==FVM_OK);assert(read(other,value.object,7).number==43);assert(read(other,value.object,0).number==9);fvm_channel_destroy(channel);
    Bytes global_body;global(global_body,"_global");FVM_Object *globals=run(c,global_body).object;write(c,globals,7,77);assert(fvm_get_global(c,"7",&value)==FVM_OK && value.number==77);assert(read(c,globals,7).number==77);
    FVM_Object *boxed=0;fvm_value_number(&value,1);assert(fvm_construct(c,"Number",1,&value,&boxed)==FVM_OK);fvm_value_number(&value,88);assert(fvm_object_set(c,boxed,"7",&value)==FVM_OK);assert(read(c,boxed,7).number==88);write(c,boxed,7,89);assert(fvm_object_get(c,boxed,"7",&value)==FVM_OK && value.number==89);
    FVM_Object *clip=0;assert(fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK);write(c,clip,7,90);assert(fvm_object_get(c,clip,"7",&value)==FVM_OK && value.number==90);
    /* Explicit native tables retain their distinct integer and named C-extension slots. */
    FVM_Object *table=0;assert(fvm_object_create(c,&table)==FVM_OK);fvm_value_number(&value,100);assert(fvm_table_set_index(c,table,7,&value)==FVM_OK);fvm_value_number(&value,200);assert(fvm_object_set(c,table,"7",&value)==FVM_OK);assert(read(c,table,7).number==100);assert(fvm_object_get(c,table,"7",&value)==FVM_OK && value.number==200);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
