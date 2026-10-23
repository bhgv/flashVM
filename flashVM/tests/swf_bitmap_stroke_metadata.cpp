#include "swf_bitmap_stroke_fixture.hpp"
static FVM_Object *record(FVM_Object *root)
{
    FVM_Object *defs=root->resource_result.object->properties["library"].object->properties["sprites"].object;
    FVM_Object *part=defs->indexed[9].object->properties["paths"].object->indexed[0].object;
    assert(part->properties.find("bitmap")==part->properties.end());
    FVM_Object *paint=part->properties["lineBitmap"].object;
    assert(paint->properties["source"].object==defs->indexed[7].object->properties["bitmap_data"].object); return paint;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0x40;kind<=0x43;++kind) {
        FVM_Object *root=load(c,bs_tags(bs_shape(kind)),1); tick(c,root); FVM_Object *paint=record(root);
        FVM_Value saved=paint->properties["repeat"],bad; fvm_value_number(&bad,2); paint->properties["repeat"]=bad;
        unsigned char pixels[1024],prior[1024]; std::memset(pixels,41,sizeof(pixels)); std::memcpy(prior,pixels,sizeof(pixels)); FVM_RasterSurface s={pixels,16,16,64};
        assert(fvm_movieclip_render(c,root,&s)==FVM_INVALID_ARGUMENT); assert(std::memcmp(pixels,prior,sizeof(pixels))==0);
        paint->properties["repeat"]=saved; bs_pixels(c,root,kind);
        FVM_Object *image=paint->properties["source"].object; FVM_Value args[3],out;
        fvm_value_number(args,0); fvm_value_number(args+1,0); fvm_value_number(args+2,0xffabcdefUL);
        assert(fvm_object_call(c,image,"setPixel32",3,args,&out)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK);
        std::memset(pixels,0,sizeof(pixels)); FVM_Result result=fvm_movieclip_render(c,root,&s);
        if(result==FVM_OK && (kind==0x40 || kind==0x42)) { const unsigned char *p=pixels+4*(8*16+4); assert(p[0]==0xab && p[1]==0xcd && p[2]==0xef && p[3]==255); }
        else assert(result==FVM_OK || result==FVM_UNSUPPORTED);
        assert(fvm_object_call(c,image,"dispose",0,0,&out)==FVM_OK); std::memset(pixels,41,sizeof(pixels));
        result=fvm_movieclip_render(c,root,&s); assert(result==FVM_OK || result==FVM_UNSUPPORTED);
        assert(std::memcmp(pixels,prior,sizeof(pixels))==0);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
