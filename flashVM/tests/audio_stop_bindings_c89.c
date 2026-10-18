/* Strict C89 header/layout and prepared-output stopping without a PCM write. */
#include "flashvm/audio_stop.h"
#include "flashvm/multimedia.h"
#include <assert.h>
static FVM_Result stop(void *raw) { ++*(unsigned *)raw; return FVM_OK; }
int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime=0;
    FVM_Context *context=0;
    FVM_AudioStopBinding *binding=0;
    unsigned calls=0;
    assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&context)==FVM_OK);
    assert(fvm_audio_stop_binding_create(context,stop,&calls,&binding)==FVM_OK && !calls);
    assert(fvm_audio_stop_all(context)==FVM_OK && calls==1);
    assert(fvm_audio_stop_binding_close(binding)==FVM_OK && calls==1);
    fvm_context_destroy(context);assert(calls==1);
    fvm_runtime_destroy(runtime);
    return 0;
}
