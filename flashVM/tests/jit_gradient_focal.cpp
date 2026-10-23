/* 144 focal configurations execute successful and boundary-rejected AVM1 calls in interpreter/manual/native JIT. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "gradient_focal_fixture.hpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned stroke=0;stroke<2;++stroke)for(unsigned mode=0;mode<3;++mode)for(unsigned interpolation=0;interpolation<2;++interpolation)for(unsigned sign=0;sign<2;++sign)for(class_mode=0;class_mode<3;++class_mode)
    {
        const double focal=sign?0.5:-0.5;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_Value args[8],out;focal_arguments(c,args,mode,focal,interpolation);if(stroke){FVM_Value width;fvm_value_number(&width,4);assert(fvm_object_call(c,clip,"lineStyle",1,&width,&out)==FVM_OK);}fvm_value_number(&out,focal);assert(fvm_set_global(c,"paintFocal",&out)==FVM_OK);ref(&out,clip);assert(fvm_set_global(c,"paintClip",&out)==FVM_OK);const char *names[]={"paintColors","paintAlphas","paintRatios","paintMatrix"};for(unsigned i=0;i<4;++i)assert(fvm_set_global(c,names[i],args+i+1)==FVM_OK);
        Program code;code.variable("paintFocal");code.text(interpolation?"linearRGB":"RGB");code.text(mode==1?"reflect":mode==2?"repeat":"pad");for(unsigned i=4;i;--i)code.variable(names[i-1]);code.text("radial");code.number(8);code.variable("paintClip");code.text(stroke?"lineGradientStyle":"beginGradientFill");code.op(0x52);code.op(0x17);code.number(42);code.finish();out=class_run(c,code);assert(out.type==FVM_VALUE_NUMBER && out.number==42);
        FVM_Object *record=clip->resource_result.object->properties[stroke?"lineGradient":"gradient"].object;assert(record && record->properties["focal"].number==focal);FVM_Value boundary;fvm_value_number(&boundary,1);assert(fvm_set_global(c,"paintFocal",&boundary)==FVM_OK);class_run(c,code,FVM_UNSUPPORTED);assert(clip->resource_result.object->properties[stroke?"lineGradient":"gradient"].object==record);if(mode)assert(record->properties["spread"].number==mode);else assert(record->properties.find("spread")==record->properties.end());if(stroke){line_gradient_geometry(c,clip);focal_pixels(c,clip,mode,focal,true,interpolation);}else{geometry(c,clip);focal_pixels(c,clip,mode,focal,false,interpolation);}assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
