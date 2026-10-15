/* The forced C service bypasses script overrides and rejects foreign/nonclip operands. */
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include <assert.h>
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;unsigned mode;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(mode=0;mode<6;++mode)
    {
        FVM_Context *c,*other;FVM_Object *root,*clip,*plain,*foreign;FVM_Value args[2],out,prototype;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        unsigned depth_mode;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        if(mode){swf[3]=(unsigned char)(mode+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
        assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK);
        assert(fvm_gc_pin(c,root)==FVM_OK && fvm_object_create(c,&plain)==FVM_OK);
        assert(fvm_standard_install(other)==FVM_OK && fvm_movieclip_create(other,"foreign",1,&foreign)==FVM_OK);
        assert(fvm_exec_detach_clip(0,root)==FVM_INVALID_ARGUMENT);
        assert(fvm_exec_detach_clip(c,0)==FVM_INVALID_ARGUMENT);
        assert(fvm_exec_detach_clip(c,plain)==FVM_INVALID_ARGUMENT);
        assert(fvm_exec_detach_clip(c,foreign)==FVM_INVALID_ARGUMENT);
        assert(fvm_get_global(c,"MovieClip",&out)==FVM_OK);
        assert(fvm_object_get(c,out.object,"prototype",&prototype)==FVM_OK);
        assert(fvm_exec_detach_clip(c,prototype.object)==FVM_INVALID_ARGUMENT);
        for(depth_mode=0;depth_mode<2;++depth_mode)
        {
            assert(fvm_value_string(args,"child")==FVM_OK);fvm_value_number(args+1,depth_mode?-16383:2);
            assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);clip=out.object;
            assert(fvm_gc_pin(c,clip)==FVM_OK);fvm_value_number(&out,99);
            assert(fvm_object_set(c,clip,"removeMovieClip",&out)==FVM_OK);
            assert(fvm_exec_detach_clip(c,clip)==FVM_OK && fvm_exec_detach_clip(c,clip)==FVM_OK);
            assert(fvm_object_call(c,root,"getInstanceAtDepth",1,args+1,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            assert(fvm_object_get(c,root,"child",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(fvm_exec_detach_clip(c,root)==FVM_OK);
        fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
