/* Host/Flash8 modes stage copied descriptors; older SWF/nonimplemented options preserve current drawing state. */
#include "gradient_spread_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);const unsigned versions[]={0,6,7,8};
    for(unsigned v=0;v<4;++v)for(unsigned mode=1;mode<=2;++mode)for(unsigned radial=0;radial<2;++radial)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);if(versions[v]){unsigned char movie[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};movie[3]=(unsigned char)versions[v];assert(fvm_load_memory(c,movie,sizeof(movie))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_Value args[8],out;spread_arguments(c,args,mode,radial!=0);assert(fvm_object_call(c,clip,"beginGradientFill",5,args,&out)==FVM_OK);FVM_Object *data=clip->resource_result.object,*old=data->properties["gradient"].object;assert(old->properties.find("spread")==old->properties.end());
        const bool modern=!versions[v] || versions[v]>=8;assert(fvm_object_call(c,clip,"beginGradientFill",8,args,&out)==(modern?FVM_OK:FVM_UNSUPPORTED));FVM_Object *record=data->properties["gradient"].object;if(modern)assert(record!=old && record->properties["spread"].number==mode);else assert(record==old);
        assert(fvm_value_string_bytes(args+5,"repeat\0ignored",14)==FVM_OK && fvm_object_call(c,clip,"beginGradientFill",6,args,&out)==FVM_UNSUPPORTED && data->properties["gradient"].object==record);
        geometry(c,clip);spread_pixels(c,clip,modern?mode:0,radial!=0);assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
