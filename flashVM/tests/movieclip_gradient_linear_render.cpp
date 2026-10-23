/* Managed fill/stroke optional interpolation selects the C89 policy and missing metadata retains RGB. */
#include "gradient_linear_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    for(unsigned stroke=0;stroke<2;++stroke)for(unsigned mode=0;mode<3;++mode)for(unsigned radial=0;radial<2;++radial){FVM_Object *clip=0;assert(fvm_movieclip_create(c,"linear",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);linear_style(c,clip,mode,radial!=0,stroke!=0);if(stroke)line_gradient_geometry(c,clip);else geometry(c,clip,false);FVM_Object *data=clip->resource_result.object,*record=data->properties[stroke?"lineGradient":"gradient"].object;linear_pixels(c,clip,mode,radial!=0,stroke!=0);assert(fvm_gc_collect(c,0)==FVM_OK);record->properties.erase("interpolation");linear_pixels(c,clip,mode,radial!=0,stroke!=0,0);set(c,record,"interpolation",0);linear_pixels(c,clip,mode,radial!=0,stroke!=0,0);set(c,record,"interpolation",1);linear_pixels(c,clip,mode,radial!=0,stroke!=0);FVM_Value out;assert(fvm_object_call(c,clip,"endFill",0,0,&out)==FVM_OK);linear_pixels(c,clip,mode,radial!=0,stroke!=0);assert(!c->gc_frames && !c->gc_native_depth);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
