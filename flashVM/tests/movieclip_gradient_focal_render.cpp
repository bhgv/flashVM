/* Managed focal fill/stroke rendering retains interpolation/spread and defaults missing focal to center. */
#include "gradient_focal_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    for(unsigned stroke=0;stroke<2;++stroke)for(unsigned mode=0;mode<3;++mode)for(unsigned interpolation=0;interpolation<2;++interpolation)for(unsigned sign=0;sign<2;++sign){const double focal=sign?0.5:-0.5;FVM_Object *clip=0;assert(fvm_movieclip_create(c,"focal",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);focal_style(c,clip,mode,focal,stroke!=0,interpolation);if(stroke)line_gradient_geometry(c,clip);else geometry(c,clip,false);FVM_Object *record=clip->resource_result.object->properties[stroke?"lineGradient":"gradient"].object;focal_pixels(c,clip,mode,focal,stroke!=0,interpolation);assert(fvm_gc_collect(c,0)==FVM_OK);record->properties.erase("focal");focal_pixels(c,clip,mode,0,stroke!=0,interpolation);set(c,record,"focal",0);focal_pixels(c,clip,mode,0,stroke!=0,interpolation);set(c,record,"focal",focal);focal_pixels(c,clip,mode,focal,stroke!=0,interpolation);FVM_Value out;assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);focal_pixels(c,clip,mode,focal,stroke!=0,interpolation);assert(!c->gc_frames && !c->gc_native_depth);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
