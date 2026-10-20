/* Codec-free builds keep additive APIs callable and leave existing contents unchanged. */
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include <cassert>
#ifndef FVM_HAS_IMAGE_CODECS
int main()
{
    FVM_Config config;FVM_Runtime *runtime=0;FVM_Context *c=0;FVM_Object *clip=0;
    assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_movieclip_create(c,"clip",3,&clip)==FVM_OK);
    unsigned char png[33]={137,80,78,71,13,10,26,10};
    assert(fvm_movieclip_replace_image(c,clip,png,sizeof(png),"png",3)==FVM_UNSUPPORTED);
    FVM_Value value;assert(fvm_object_get(c,clip,"_totalframes",&value)==FVM_OK && value.number==3);
    assert(fvm_movieclip_replace_resource(c,clip,png,sizeof(png),"png",3)==FVM_UNSUPPORTED);
    fvm_context_destroy(c);fvm_runtime_destroy(runtime);return 0;
}
#else
int main(){return 0;}
#endif
