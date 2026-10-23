/* Script stroke ramps use their own colors, survive endFill and reset only with lineStyle/clear. */
#include "line_gradient_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)for(unsigned radial=0;radial<2;++radial){FVM_Object *clip=0;assert(fvm_movieclip_create(c,"line",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);line_gradient_style(c,clip,mode,radial!=0);line_gradient_geometry(c,clip);FVM_Object *data=clip->resource_result.object,*record=data->properties["lineGradient"].object,*active=data->properties["active"].object;assert(active->properties["lineGradient"].object==record);line_gradient_pixels(c,clip,mode,radial!=0);FVM_Value out;assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK && data->properties["lineGradient"].object==record);line_gradient_pixels(c,clip,mode,radial!=0);FVM_Value width;fvm_value_number(&width,2);assert(fvm_object_call(c,clip,"lineStyle",1,&width,&out)==FVM_OK && data->properties.find("lineGradient")==data->properties.end() && active->properties["lineGradient"].object==record);line_gradient_pixels(c,clip,mode,radial!=0);assert(fvm_object_call(c,clip,"clear",0,0,&out)==FVM_OK && data->properties.find("lineGradient")==data->properties.end());assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
