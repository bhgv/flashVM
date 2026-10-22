/* Missing/negative font underline thickness is explicit UNSUPPORTED and never changes a field. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
/* Read a TrueType directory offset, independently of the production font facade. */
static unsigned long big32(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
int main()
{
#ifdef FVM_HAS_FONTS
 for(unsigned negative=0;negative<2;++negative){
  unsigned char bytes[sizeof(fvm_test_font)];std::memcpy(bytes,fvm_test_font,sizeof(bytes));
  size_t post=0;unsigned tables=((unsigned)bytes[4]<<8)|bytes[5];
  for(unsigned i=0;i<tables;++i){size_t at=12+16*i;assert(at+16<=sizeof(bytes));if(!std::memcmp(bytes+at,"post",4))post=(size_t)big32(bytes+at+8);}
  assert(post && post+12<=sizeof(bytes));bytes[post+10]=bytes[post+11]=negative?255:0;
  FVM_Font *font=0;FVM_FontUnderlineMetrics u;
  assert(fvm_font_read(bytes,sizeof(bytes),0,40,&font)==FVM_OK && fvm_font_underline_metrics(font,&u)==FVM_UNSUPPORTED && u.center_y==0 && u.thickness==0);fvm_font_destroy(font);
  FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
  assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
  FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out,text;
  assert(fvm_standard_text_register_font(c,"Missing",bytes,sizeof(bytes),0)==FVM_OK && fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
  assert(fvm_value_string(&value,"Missing")==FVM_OK && fvm_object_set(c,format,"font",&value)==FVM_OK);
  fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"underline",&value)==FVM_OK);
  assert(fvm_value_string(&text,"AV")==FVM_OK && fvm_object_set(c,field,"text",&text)==FVM_OK);
  double before=counted_metric(c,field,"textWidth");fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
  assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_UNSUPPORTED && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_UNSUPPORTED);
  assert(fvm_object_call(c,format,"getTextExtent",1,&text,&out)==FVM_UNSUPPORTED && counted_metric(c,field,"textWidth")==before);
  assert(fvm_object_call(c,field,"getTextFormat",0,0,&out)==FVM_OK && fvm_object_get(c,out.object,"underline",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && !value.boolean);
  fvm_context_destroy(c);fvm_runtime_destroy(rt);
 }
#endif
 return 0;
}
