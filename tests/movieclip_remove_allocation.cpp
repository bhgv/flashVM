/* Every preparation failure retains both sides of native clip ownership; stale slots are protected. */
#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_scene.hpp"
static FVM_Object *child(FVM_Context *c,FVM_Object *root,const char *name,double depth)
{
    FVM_Value args[2],out;fvm_value_string(args,name);fvm_value_number(args+1,depth);
    assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);return out.object;
}
static FVM_Value obj(FVM_Object *object)
{FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=object;return v;}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<2;++mode)
    {
        FVM_Context *c=0;FVM_Object *root=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"root",3,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        FVM_Object *clip=child(c,root,"child",mode?-16383:2);assert(fvm_gc_pin(c,clip)==FVM_OK);
        FVM_Object *data=clip->resource_result.object,*children=root->resource_result.object->properties["children"].object;
        assert(children->indexed.size()==1);unsigned long key=children->indexed.begin()->first;
        size_t bindings=root->properties.size(),fields=data->properties.size();bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<2000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;
            allowance=boundary;FVM_Result r=fvm_movieclip_scene_remove(c,clip);allowance=-1;
            assert(c->gc_frames==roots);
            if(r==FVM_OK)
            {
                assert(children->indexed.empty() && root->properties.find("child")==root->properties.end());
                assert(data->properties["parent"].type==FVM_VALUE_NULL && data->properties["playing"].number==0);success=true;
            }
            else
            {
                assert(r==FVM_OUT_OF_MEMORY && root->properties.size()==bindings && data->properties.size()==fields);
                assert(children->indexed.size()==1 && children->indexed.find(key)->second.object==clip);
                assert(root->properties.find("child")->second.object==clip);
                assert(data->properties["parent"].object==root && data->properties["playing"].number==1);++failures;
            }
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        assert(success && failures);fvm_context_destroy(c);
    }
    {
        FVM_Context *c=0;FVM_Object *root=0;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);
        FVM_Object *old=child(c,root,"old",2),*replacement=child(c,root,"replacement",2);
        old->resource_result.object->properties["parent"]=obj(root);
        FVM_Object *children=root->resource_result.object->properties["children"].object;
        assert(children->indexed.size()==1);unsigned long key=children->indexed.begin()->first;
        assert(fvm_movieclip_scene_remove(c,old)==FVM_OK && children->indexed.find(key)!=children->indexed.end());
        assert(children->indexed.find(key)->second.object==replacement);
        assert(root->properties.find("replacement")->second.object==replacement);
        assert(replacement->resource_result.object->properties["parent"].object==root);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
