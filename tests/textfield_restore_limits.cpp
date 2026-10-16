/* Oversized native text records reject after valid envelope CRC and roll back graph state. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"
#include "zlib.h"
#ifdef FVM_HAS_FONTS
static void restore_u32(unsigned char *p,unsigned long value)
{for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(value>>((3-i)*8));}
#endif
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
#ifdef FVM_HAS_FONTS
 FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);FVM_Object *field=counted_field(c);assert(fvm_standard_text_install(other)==FVM_OK);
 fvm_detail::vector<char> text;text.resize(4096);for(size_t i=0;i<text.size();++i)text[i]='Q';FVM_Value value,out,root;assert(fvm_value_string_bytes(&value,text.begin(),text.size())==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *data=0;size_t size=0;assert(fvm_value_serialize(c,&root,&data,&size)==FVM_OK);
 size_t at=0,matches=0;for(size_t i=20;i+text.size()<=size;++i)if(!std::memcmp(data+i,text.begin(),text.size())){at=i;++matches;}
 assert(matches==1 && data[at-4]==0 && data[at-3]==0 && data[at-2]==16 && data[at-1]==0);
 const size_t added[]={1,12289};
 for(unsigned test=0;test<2;++test){size_t extra=added[test];fvm_detail::vector<unsigned char> bad;bad.resize(size+extra);
  std::memcpy(bad.begin(),data,at+text.size());for(size_t i=0;i<extra;++i)bad[at+text.size()+i]='Q';std::memcpy(bad.begin()+at+text.size()+extra,data+at+text.size(),size-at-text.size());
  restore_u32(bad.begin()+at-4,(unsigned long)(text.size()+extra));restore_u32(bad.begin()+8,(unsigned long)(bad.size()-16));restore_u32(bad.begin()+12,(unsigned long)crc32(0,bad.begin()+16,(uInt)(bad.size()-16)));
  unsigned long before=0,after=0,pending=0,pending_after=0;assert(fvm_gc_object_count(other,&before)==FVM_OK && fvm_gc_pending_allocations(other,&pending)==FVM_OK);size_t fonts=other->fonts.size();
  assert(fvm_value_deserialize(other,bad.begin(),bad.size(),&out)==FVM_INVALID_ARGUMENT && out.type==FVM_VALUE_UNDEFINED);
  assert(fvm_gc_object_count(other,&after)==FVM_OK && after==before && fvm_gc_pending_allocations(other,&pending_after)==FVM_OK && pending_after==pending && other->fonts.size()==fonts && !other->gc_frames && !other->gc_native_depth);
 }
 /* The valid scalar ceiling still round-trips, including its full counted byte length. */
 assert(fvm_value_deserialize(other,data,size,&out)==FVM_OK && fvm_gc_pin(other,out.object)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);counted_text(other,out.object,text.begin(),text.size());assert(counted_metric(other,out.object,"length")==4096);fvm_serialized_free(data);
 /* The independent byte/UTF-16 ceiling accepts 4096 supplementary scalars. */
 text.resize(16384);for(size_t i=0;i<4096;++i)std::memcpy(text.begin()+i*4,"\xf0\x9f\x98\x80",4);
 assert(fvm_value_string_bytes(&value,text.begin(),text.size())==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_value_serialize(c,&root,&data,&size)==FVM_OK && fvm_value_deserialize(other,data,size,&out)==FVM_OK && fvm_gc_pin(other,out.object)==FVM_OK);
 assert(counted_metric(other,out.object,"length")==8192);counted_text(other,out.object,text.begin(),text.size());fvm_serialized_free(data);fvm_context_destroy(other);fvm_context_destroy(c);
#endif
 fvm_runtime_destroy(rt);
}
