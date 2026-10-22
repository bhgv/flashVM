/* Retired coordinate stubs upgrade while malformed policy bytes fail graph admission. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK);
 FVM_Object *field=counted_field(c);FVM_Value v,root,out;const char *text="AVAVABAB";
 assert(fvm_value_string(&v,text)==FVM_OK && fvm_object_set(c,field,"text",&v)==FVM_OK);
 FVM_NativeFunction stub=fvm_text_callback_from_id(205);const char *keys[]={"autoSize","_x","_y"};
 for(unsigned i=0;i<3;++i){FVM_AccessorEntry &entry=field->accessors[keys[i]];entry.getter=entry.setter=stub;entry.root=0;entry.userdata=0;}
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;
 assert(fvm_standard_text_install(d)==FVM_OK && fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK);
 FVM_Object *copy=out.object;assert(fvm_gc_pin(d,copy)==FVM_OK && counted_metric(d,copy,"_x")==0 && counted_metric(d,copy,"_y")==0);
 fvm_value_number(&v,-12);assert(fvm_object_set(d,copy,"_x",&v)==FVM_OK && fvm_object_set(d,copy,"_y",&v)==FVM_OK);
 assert(fvm_value_string(&v,"right")==FVM_OK && fvm_object_set(d,copy,"autoSize",&v)==FVM_OK);
 /* Independent graph-format oracle: each number is type + big-endian IEEE double + metadata reference. */
 size_t policy=0,hits=0;for(size_t i=0;i+8+26+2<size;++i)if(!std::memcmp(wire+i,text,8)){policy=i+8+26;++hits;}
 assert(hits==1 && wire[policy]==0 && wire[policy+1]==0);
 const unsigned char flags[]={8,16,26,128};
 for(unsigned i=0;i<4;++i){wire[policy]=flags[i];assert(fvm_value_deserialize(d,wire,size,&out)==FVM_INVALID_ARGUMENT);}
 wire[policy]=0;wire[policy+1]=2;assert(fvm_value_deserialize(d,wire,size,&out)==FVM_INVALID_ARGUMENT);wire[policy+1]=0;
 assert(fvm_value_deserialize(d,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);
 /* Direct restoration upgrades pre-existing stubs too, without requiring graph callback restoration. */
 FVM_TextFieldState state;assert(fvm_textfield_snapshot(copy,state)==FVM_OK);FVM_Object *direct=0;assert(fvm_object_create(d,&direct)==FVM_OK);
 for(unsigned i=0;i<3;++i){FVM_AccessorEntry &entry=direct->accessors[keys[i]];entry.getter=entry.setter=stub;}
 direct->resource_kind=FVM_RESOURCE_TEXT_FIELD;assert(fvm_textfield_restore(d,direct,state)==FVM_OK && counted_metric(d,direct,"_y")==-12);
 for(unsigned i=0;i<3;++i){FVM_AccessorEntry captured;captured.getter=captured.setter=stub;captured.root=copy;fvm_textfield_upgrade_accessor(keys[i],captured);assert(captured.getter==stub && captured.root==copy);}
 fvm_context_destroy(c);assert(fvm_gc_collect(d,0)==FVM_OK);fvm_context_destroy(d);fvm_runtime_destroy(rt);
#endif
 return 0;
}
