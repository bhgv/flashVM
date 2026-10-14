/* C89 TargetPath returns counted native paths without user string/target overrides. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
static FVM_Result forbidden(FVM_Context *c,FVM_Object *o,unsigned n,const FVM_Value *a,FVM_Value *v,void *p)
{(void)c;(void)o;(void)n;(void)a;(void)v;(void)p;assert(0);return FVM_ERROR;}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;unsigned version;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(version=4;version<=8;++version)
    {
        FVM_Context *c;FVM_Object *root,*child,*plain;FVM_Value input,out,args[2];unsigned i;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"host-root-name",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        fvm_value_undefined(&input);input.type=FVM_VALUE_OBJECT;input.object=root;
        assert(fvm_exec_target_path(c,&input,&out)==FVM_OK && out.type==FVM_VALUE_STRING && !strcmp(out.string,"_level0"));
        assert(fvm_value_string(args,"child")==FVM_OK);fvm_value_number(args+1,1);
        assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);child=out.object;
        assert(fvm_object_define_method_rooted(c,child,"toString",forbidden,0,0)==FVM_OK);
        assert(fvm_object_define_accessor_rooted(c,child,"_target",forbidden,0,0,0)==FVM_OK);
        input.object=child;
        assert(fvm_exec_target_path(c,&input,&out)==FVM_OK && !strcmp(out.string,"_level0.child"));
        assert(fvm_value_string_bytes(args,"x\0y",3)==FVM_OK && fvm_object_set(c,child,"_name",args)==FVM_OK);
        assert(fvm_exec_target_path(c,&input,&input)==FVM_OK && fvm_value_string_size(&input)==11);
        assert(!memcmp(input.string,"_level0.x\0y",11));
        assert(fvm_object_create(c,&plain)==FVM_OK && fvm_object_define_method_rooted(c,plain,"toString",forbidden,0,0)==FVM_OK);
        for(i=0;i<6;++i)
        {
            fvm_value_undefined(&input);
            if(i==1)input.type=FVM_VALUE_NULL;
            if(i==2)fvm_value_number(&input,17);
            if(i==3)fvm_value_boolean(&input,1);
            if(i==4)assert(fvm_value_string(&input,"_level0")==FVM_OK);
            if(i==5){input.type=FVM_VALUE_OBJECT;input.object=plain;}
            assert(fvm_exec_target_path(c,&input,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        }
        assert(fvm_get_global(c,"MovieClip",&input)==FVM_OK && fvm_object_get(c,input.object,"prototype",&input)==FVM_OK);
        assert(fvm_exec_target_path(c,&input,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        fvm_value_number(&out,99);
        assert(fvm_exec_target_path(0,&input,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_target_path(c,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_target_path(c,&input,0)==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
