/* Replacing the live handler table inside bytecode does not invalidate this delivery's saved batch. */
#include "clip_actions_fixture.hpp"
static FVM_Object *target,*empty_table,*old_table;
static unsigned calls;
static FVM_Result replace_table(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++calls;FVM_Value corrupt;fvm_value_number(&corrupt,0);old_table->indexed[1].object->properties["events"]=corrupt;old_table->indexed[1].object->indexed[0]=corrupt;FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=empty_table;target->resource_result.object->properties["clip_actions"]=value;assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_undefined(out);return FVM_OK;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=clip_context(rt);Bytes code;integer(code,0);text(code,"replaceTable");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);Bytes events;u16(events,0);u32(events,1);clip_record(events,8,1,code);clip_record(events,8,1,action(42));u32(events,0);
 FVM_Object *root=clip_load(c,clip_movie(8,events));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);target=get(c,root,"actor").object;old_table=target->resource_result.object->properties["clip_actions"].object;assert(fvm_object_create(c,&empty_table)==FVM_OK && fvm_gc_pin(c,empty_table)==FVM_OK && fvm_register_function(c,"replaceTable",replace_table,0)==FVM_OK);
 calls=0;assert(fvm_movieclip_dispatch_clip_actions(c,target,1,0)==FVM_OK && calls==1);number(c,target,"seen",42);FVM_Value value;fvm_value_number(&value,0);assert(fvm_object_set(c,target,"seen",&value)==FVM_OK && fvm_movieclip_dispatch_clip_actions(c,target,1,0)==FVM_OK && calls==1);number(c,target,"seen",0);clip_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
