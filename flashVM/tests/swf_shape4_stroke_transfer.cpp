#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned int mode=0;mode<12;++mode) {
        FVM_Context *sender=0,*binary=0,*channel=0;
        assert(fvm_context_create(rt,&sender)==FVM_OK); assert(fvm_context_create(rt,&binary)==FVM_OK);
        assert(fvm_context_create(rt,&channel)==FVM_OK); assert(fvm_standard_install(sender)==FVM_OK);
        unsigned int spread=mode%3, interpolation=(mode/3)%2; int focal=mode<6?-255:255;
        FVM_Object *root=load(sender,shape4_tags(shape4_stroke(0x13,spread,interpolation,focal,80,2,8,4,true)),1); tick(sender,root);
        FVM_Value value,copy; fvm_value_undefined(&value); value.type=FVM_VALUE_OBJECT; value.object=root;
        unsigned char *bytes=0; size_t size=0; assert(fvm_value_serialize(sender,&value,&bytes,&size)==FVM_OK);
        assert(fvm_value_deserialize(binary,bytes,size,&copy)==FVM_OK); fvm_serialized_free(bytes);
        FVM_Object *b=copy.object; assert(fvm_gc_pin(binary,b)==FVM_OK);
        FVM_Channel *ch=0; assert(fvm_channel_create(&ch)==FVM_OK);
        assert(fvm_channel_send_value(sender,ch,&value,0)==FVM_OK);
        assert(fvm_channel_receive_value(channel,ch,&copy,0)==FVM_OK); FVM_Object *q=copy.object;
        assert(fvm_gc_pin(channel,q)==FVM_OK); fvm_channel_destroy(ch); fvm_context_destroy(sender);
        assert(fvm_gc_collect(binary,0)==FVM_OK); assert(fvm_gc_collect(channel,0)==FVM_OK);
        stroke4_pixels(binary,b,0x13,spread,interpolation,focal/256.0,80,2,true,true);
        stroke4_pixels(channel,q,0x13,spread,interpolation,focal/256.0,80,2,true,true);
        fvm_context_destroy(binary); fvm_context_destroy(channel);
    }
    fvm_runtime_destroy(rt); return 0;
}
