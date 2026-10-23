#include "script_stroke_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned kind=0;kind<4;++kind) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) { FVM_Context *c=0,*d=0,*e=0; ss_context(rt,&c); assert(fvm_context_create(rt,&d)==FVM_OK && fvm_context_create(rt,&e)==FVM_OK);
  FVM_Object *clip=ss_clip(c,cap,join,kind); FVM_Value value,out; ref(&value,clip); unsigned char *wire=0; size_t size=0; assert(fvm_value_serialize(c,&value,&wire,&size)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK); fvm_serialized_free(wire); FVM_Object *binary=out.object; assert(fvm_gc_pin(d,binary)==FVM_OK);
  FVM_Channel *ch=0; assert(fvm_channel_create(&ch)==FVM_OK && fvm_channel_send_value(c,ch,&value,0)==FVM_OK && fvm_channel_receive_value(e,ch,&out,0)==FVM_OK); FVM_Object *received=out.object; assert(fvm_gc_pin(e,received)==FVM_OK); fvm_channel_destroy(ch); fvm_context_destroy(c);
  assert(fvm_gc_collect(d,0)==FVM_OK && fvm_gc_collect(e,0)==FVM_OK); ss_metadata(binary,cap,join); ss_metadata(received,cap,join); ss_pixels(d,binary,cap,join,kind); ss_pixels(e,received,cap,join,kind);
  fvm_context_destroy(d); fvm_context_destroy(e);
 }
 fvm_runtime_destroy(rt); return 0;
}
