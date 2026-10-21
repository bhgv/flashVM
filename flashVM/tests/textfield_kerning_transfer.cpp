/* Current/future kerning flags survive binary/channel transfer; legacy field alignment retains kerned geometry. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
#include <cmath>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value arg,value,out,root;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
 fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,format,"kerning",&value)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && (state.current.alignment&FVM_TEXT_STYLE_NO_KERNING) && !(state.future.alignment&FVM_TEXT_STYLE_NO_KERNING));
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;
 assert(fvm_standard_text_install(other)==FVM_OK);
 unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(other,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);
 assert(fvm_gc_pin(other,out.object)==FVM_OK && std::fabs(counted_metric(other,out.object,"textWidth")-48)<0.02);
 FVM_Object *copy=out.object;
 assert(fvm_value_string(&value,"AVA")==FVM_OK && fvm_object_set(other,copy,"text",&value)==FVM_OK && std::fabs(counted_metric(other,copy,"textWidth")-68.8)<0.02);
 FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&root,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);fvm_channel_destroy(channel);
 assert(std::fabs(counted_metric(other,out.object,"textWidth")-48)<0.02);
 /* A pre-feature field record has no flag and must retain its old implicit kerning. */
 state.current.alignment&=~FVM_TEXT_STYLE_NO_KERNING;
 FVM_Object *legacy=0;assert(fvm_object_create(other,&legacy)==FVM_OK);
 legacy->resource_kind=copy->resource_kind;legacy->accessors=copy->accessors;legacy->methods=copy->methods;
 assert(fvm_textfield_restore(other,legacy,state)==FVM_OK && std::fabs(counted_metric(other,legacy,"textWidth")-44.8)<0.02);
 state.current.alignment=8;FVM_Object *bad=0;assert(fvm_object_create(other,&bad)==FVM_OK && fvm_textfield_restore(other,bad,state)==FVM_INVALID_ARGUMENT);
 fvm_context_destroy(c);assert(fvm_gc_collect(other,0)==FVM_OK && std::fabs(counted_metric(other,copy,"textWidth")-68.8)<0.02);
 fvm_context_destroy(other);fvm_runtime_destroy(rt);
#endif
 return 0;
}
