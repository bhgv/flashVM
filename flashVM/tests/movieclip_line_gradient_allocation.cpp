/* Fault sweeps preserve the prior active path/style and prepared raster pixels until successful publication. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#include "line_gradient_fixture.hpp"
int main(int argc,char **)
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=1;mode<=2;++mode)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_Value args[8],out;spread_arguments(c,args,mode);FVM_Value width;fvm_value_number(&width,4);assert(fvm_object_call(c,clip,"lineStyle",1,&width,&out)==FVM_OK);assert(fvm_object_call(c,clip,"lineGradientStyle",5,args,&out)==FVM_OK);FVM_Value point[2];fvm_value_number(point,1);fvm_value_number(point+1,8);assert(fvm_object_call(c,clip,"moveTo",2,point,&out)==FVM_OK);FVM_Object *data=clip->resource_result.object,*old=data->properties["lineGradient"].object,*active=data->properties["active"].object;bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<4096 && !success;++boundary){allowance=argc>1?-1:boundary;FVM_Result result=fvm_object_call(c,clip,"lineGradientStyle",8,args,&out);allowance=-1;assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success){++failures;assert(data->properties["lineGradient"].object==old && data->properties["active"].object==active);}else assert(data->properties["lineGradient"].object->properties["spread"].number==mode);assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);}
        assert(success && (argc>1 || failures));line_gradient_geometry(c,clip);line_gradient_pixels(c,clip,mode);fvm_context_destroy(c);
    }
    FVM_Path *path=0;assert(fvm_path_create(&path)==FVM_OK);FVM_PathCommand command;std::memset(&command,0,sizeof(command));const double xy[]={0,0,16,0,16,16,0,16};for(unsigned i=0;i<8;i+=2){command.verb=i?FVM_PATH_LINE:FVM_PATH_MOVE;command.x[0]=xy[i];command.y[0]=xy[i+1];assert(fvm_path_add(path,&command)==FVM_OK);}command.verb=FVM_PATH_CLOSE;assert(fvm_path_add(path,&command)==FVM_OK);
    FVM_Gradient gradient;assert(fvm_gradient_init(&gradient)==FVM_OK);FVM_GradientStop stops[2];stops[0].ratio=0;stops[0].color.r=stops[0].color.a=255;stops[0].color.g=stops[0].color.b=0;stops[1].ratio=1;stops[1].color.r=stops[1].color.g=0;stops[1].color.b=stops[1].color.a=255;gradient.stops=stops;gradient.count=2;gradient.matrix.xx=gradient.matrix.yy=2;
    for(unsigned type=0;type<2;++type)for(unsigned mode=1;mode<=2;++mode){gradient.type=(FVM_GradientType)type;bool success=false,supported=true;unsigned failures=0;for(long boundary=0;boundary<4096 && !success;++boundary){unsigned char pixels[1024],saved[1024];std::memset(pixels,37,sizeof(pixels));std::memcpy(saved,pixels,sizeof(pixels));FVM_RasterSurface surface={pixels,16,16,64};allowance=argc>1?-1:boundary;FVM_Result result=fvm_path_stroke_gradient(path,&surface,&gradient,4,mode);allowance=-1;assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY || result==FVM_UNSUPPORTED);success=result!=FVM_OUT_OF_MEMORY;supported=result!=FVM_UNSUPPORTED;if(!success){++failures;assert(!std::memcmp(pixels,saved,sizeof(pixels)));}}assert(success && (!supported || argc>1 || failures));}fvm_path_destroy(path);
    fvm_runtime_destroy(rt);return 0;
}
