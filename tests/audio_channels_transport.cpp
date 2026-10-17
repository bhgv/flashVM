/* Transported stopAllSounds retains native callback identity and affects only the destination. */
#define main audio_channels_fixture_main
#include "audio_channels.c"
#undef main
#include "flashvm/function.h"
#include "flashvm/serialization.h"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *source=0,*target=0;Host a,b;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 assert(fvm_context_create(rt,&source)==FVM_OK && fvm_context_create(rt,&target)==FVM_OK);
 assert(fvm_standard_install(source)==FVM_OK && fvm_standard_install(target)==FVM_OK);
 create(source,&a);create(target,&b);unsigned char data[8]={0};FVM_MediaFrame frame;memset(&frame,0,sizeof(frame));frame.kind=FVM_MEDIA_AUDIO;frame.data=data;frame.size=sizeof(data);frame.channels=2;frame.sample_rate=8000;frame.samples=2;
 assert(fvm_audio_channel_write(a.channel,&frame)==FVM_OK && fvm_audio_channel_write(b.channel,&frame)==FVM_OK);
 FVM_Value function,copy,out;unsigned char *wire=0;size_t length=0;
 assert(fvm_get_global(source,"stopAllSounds",&function)==FVM_OK && function.type==FVM_VALUE_OBJECT);
 assert(fvm_value_serialize(source,&function,&wire,&length)==FVM_OK);
 assert(fvm_value_deserialize(target,wire,length,&copy)==FVM_OK);fvm_serialized_free(wire);
 assert(fvm_function_invoke(target,copy.object,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
 assert(!a.stops && b.stops==1);assert(fvm_audio_channel_close(a.channel)==FVM_OK && a.stops==1);
 assert(fvm_audio_channel_close(b.channel)==FVM_OK && b.stops==1);
 fvm_context_destroy(source);fvm_context_destroy(target);fvm_runtime_destroy(rt);return 0;
}
