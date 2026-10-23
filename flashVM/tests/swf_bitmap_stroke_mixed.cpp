#include "swf_bitmap_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0x40;kind<=0x43;++kind) {
        size_t style=0,end=0; Bytes fill=bs_shape(kind,80,false,false,0,7,&style,&end);
        Bytes stroke=bs_shape(0x40+(0x43-kind),80,false,false,0,8),bytes;
        for(size_t i=0;i<style-2;++i) bytes.push_back(fill[i]); bytes.push_back(1);
        for(size_t i=style+4;i<end;++i) bytes.push_back(fill[i]); bytes.push_back(1);
        for(size_t i=style;i<end;++i) bytes.push_back(stroke[i]);
        Bits records; records.put(1,4); records.put(1,4); move(records,0,0,true);
        edge(records,320,0); edge(records,0,320); edge(records,-320,0); edge(records,0,-320); records.put(0,6); append(bytes,records.b);
        Bytes tags,empty,green; for(unsigned i=0;i<4;++i) { green.push_back(255); green.push_back(0); green.push_back(255); green.push_back(0); }
        tag(tags,36,lossless(5,2,2,bs_raw())); tag(tags,36,lossless(5,2,2,green,0,8)); tag(tags,83,bytes);
        tag(tags,26,place(6,1,9,0,0)); tag(tags,1,empty); tag(tags,0,empty); FVM_Object *root=load(c,tags,1); tick(c,root);
        FVM_Object *defs=root->resource_result.object->properties["library"].object->properties["sprites"].object;
        FVM_Object *paints=defs->indexed[9].object->properties["paths"].object;
        assert(paints->indexed[0].object->properties["bitmap"].object->properties["source"].object==defs->indexed[7].object->properties["bitmap_data"].object);
        assert(paints->indexed[1].object->properties["lineBitmap"].object->properties["source"].object==defs->indexed[8].object->properties["bitmap_data"].object);
        unsigned char actual[1024],expected[1024]; std::memset(actual,0,sizeof(actual)); std::memset(expected,0,sizeof(expected));
        FVM_RasterSurface a={actual,16,16,64},b={expected,16,16,64}; FVM_Result result=fvm_movieclip_render(c,root,&a); if(result==FVM_UNSUPPORTED) continue; assert(result==FVM_OK);
        FVM_Path *path=0; assert(fvm_path_create(&path)==FVM_OK); FVM_PathCommand cmd; std::memset(&cmd,0,sizeof(cmd)); cmd.verb=FVM_PATH_MOVE;
        assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.verb=FVM_PATH_LINE; cmd.x[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK);
        cmd.y[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.x[0]=0; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.y[0]=0;
        assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(path,&cmd)==FVM_OK);
        const unsigned char rgba[]={255,0,0,128,0,255,0,255,0,0,0,0,0,0,255,64}; FVM_RasterSurface image={(unsigned char*)rgba,2,2,8};
        FVM_BitmapPaint paint; assert(fvm_bitmap_paint_init(&paint)==FVM_OK); paint.source=&image; paint.repeat=(kind==0x40 || kind==0x42); paint.smoothing=kind<0x42;
        assert(fvm_path_draw_bitmap(path,&b,&paint,1)==FVM_OK); FVM_RasterColor color={0,255,0,255}; assert(fvm_path_draw(path,&b,color,4,0)==FVM_OK);
        assert(std::memcmp(actual,expected,sizeof(actual))==0); fvm_path_destroy(path);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
