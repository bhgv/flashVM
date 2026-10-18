/* Reload and transport keep legacy and sensitive constructor registrations independent. */
#define FVM_CLASS_CASE_FIXTURE_ONLY
#include "object_register_class_case.cpp"
static void env_query(FVM_Context *c,const char *name,FVM_Object *wanted)
{FVM_Object *found=0;assert(fvm_registered_class(c,name,&found)==(wanted?FVM_OK:FVM_NOT_FOUND));if(wanted)assert(found==wanted);}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Value klass;assert(fvm_get_global(c,"Object",&klass)==FVM_OK);
 FVM_Object *one=case_ctor(c,11),*two=case_ctor(c,22),*three=case_ctor(c,33);case_register(c,klass.object,"Hero",4,one);
 for(unsigned v=4;v<=6;++v){case_loaded(c,v);env_query(c,"Hero",0);case_register(c,klass.object,"hERO",4,two);env_query(c,"HERO",two);case_register(c,klass.object,"hero",4,0);}
 case_loaded(c,6);case_register(c,klass.object,"hERO",4,two);case_register(c,klass.object,"0",1,two);case_register(c,klass.object,"A\0B",3,two);
 for(unsigned v=7;v<=8;++v){case_loaded(c,v);env_query(c,"Hero",one);env_query(c,"hERO",0);case_register(c,klass.object,"hERO",4,three);env_query(c,"Hero",one);env_query(c,"hERO",three);case_register(c,klass.object,"hERO",4,0);}
 case_loaded(c,8);case_register(c,klass.object,"hERO",4,three);case_register(c,klass.object,"0",1,one);case_register(c,klass.object,"A\0B",3,one);
 assert(fvm_gc_collect(c,0)==FVM_OK);case_loaded(c,6);env_query(c,"HERO",two);env_query(c,"0",two);FVM_Object *found=0;assert(fvm_registered_class_bytes(c,"a\0b",3,&found)==FVM_OK && found==two);
 case_register(c,klass.object,"hero",4,0);case_loaded(c,8);env_query(c,"Hero",one);env_query(c,"hERO",three);case_loaded(c,6);case_register(c,klass.object,"Hero",4,two);
 /* Both environments travel with the same captured Object.registerClass registry. */
 FVM_Channel *ch=0;FVM_Context *other=0;FVM_Value copy;assert(fvm_channel_create(&ch)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 assert(fvm_channel_send_value(c,ch,&klass,0)==FVM_OK && fvm_channel_receive_value(other,ch,&copy,0)==FVM_OK && fvm_set_global(other,"Object",&copy)==FVM_OK);
 for(unsigned v=4;v<=8;++v){case_loaded(other,v);assert(fvm_gc_collect(other,0)==FVM_OK);assert(fvm_registered_class(other,v<7?"HERO":"Hero",&found)==FVM_OK);FVM_Value out;assert(fvm_function_invoke(other,found,0,0,0,&out)==FVM_OK && out.number==(v<7?22:11));}
 case_loaded(other,6);case_register(other,copy.object,"HERO",4,0);env_query(other,"hero",0);case_loaded(other,8);assert(fvm_registered_class(other,"Hero",&found)==FVM_OK);
 case_loaded(c,6);env_query(c,"Hero",two);assert(!c->gc_frames && !c->gc_native_depth && !other->gc_frames && !other->gc_native_depth);
 /* Older flat records still use their historical map instead of losing saved entries. */
 FVM_Object *flat=0;assert(fvm_object_create(c,&flat)==FVM_OK);flat->properties["Old"]=case_object(one);fvm_detail::string key="OLD";assert(fvm_base_registered_class(c,flat,key,&found)==FVM_OK && found==one);
 fvm_channel_destroy(ch);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
