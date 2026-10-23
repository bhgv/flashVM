/* Seventy-two real AVM1 linearRGB gradient calls compare interpreter/manual/native JIT with native-entry evidence. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "gradient_linear_fixture.hpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned stroke=0;stroke<2;++stroke)for(unsigned mode=0;mode<3;++mode)for(unsigned radial=0;radial<2;++radial)for(class_mode=0;class_mode<3;++class_mode)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_Value args[8],out;linear_arguments(c,args,mode,radial!=0);if(stroke){FVM_Value width;fvm_value_number(&width,4);assert(fvm_object_call(c,clip,"lineStyle",1,&width,&out)==FVM_OK);}ref(&out,clip);assert(fvm_set_global(c,"paintClip",&out)==FVM_OK);const char *names[]={"paintColors","paintAlphas","paintRatios","paintMatrix"};for(unsigned i=0;i<4;++i)assert(fvm_set_global(c,names[i],args+i+1)==FVM_OK);
        Program code;code.number(0);code.text("linearRGB");code.text(mode==1?"reflect":mode==2?"repeat":"pad");for(unsigned i=4;i;--i)code.variable(names[i-1]);code.text(radial?"radial":"linear");code.number(8);code.variable("paintClip");code.text(stroke?"lineGradientStyle":"beginGradientFill");code.op(0x52);code.op(0x17);code.number(42);code.finish();out=class_run(c,code);assert(out.type==FVM_VALUE_NUMBER && out.number==42);
        FVM_Object *record=clip->resource_result.object->properties[stroke?"lineGradient":"gradient"].object;assert(record && record->properties["interpolation"].number==1);if(mode)assert(record->properties["spread"].number==mode);else assert(record->properties.find("spread")==record->properties.end());if(stroke){line_gradient_geometry(c,clip);linear_pixels(c,clip,mode,radial!=0,true);}else{geometry(c,clip);linear_pixels(c,clip,mode,radial!=0,false);}assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
