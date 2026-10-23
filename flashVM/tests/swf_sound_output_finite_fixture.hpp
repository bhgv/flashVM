/* Prepared native finite event owner and injectable FIFO after complete source VM teardown. */
#ifndef FVM_TEST_OUTPUT_FINITE_FIXTURE_HPP
#define FVM_TEST_OUTPUT_FINITE_FIXTURE_HPP
#include "swf_sound_output_fixture.hpp"
struct FiniteOutput
{
 output_fixture::Device device;FVM_SWFSoundMixer *mixer;FVM_SWFSoundOutput *output;FVM_SWFSoundVoice *templates[2];
 FiniteOutput(unsigned frames=8,bool tracking=false){mixer=0;output=0;templates[0]=templates[1]=0;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK&&fvm_context_create(rt,&c)==FVM_OK);conversion_fixture::Samples samples;for(unsigned i=0;i<frames;++i)samples.push_back((short)(2000+i*17));conversion_fixture::Bytes swf=conversion_fixture::movie(samples,3,1);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);for(unsigned i=0;i<2;++i)assert(fvm_swf_sound_voice_open(c,i,templates+i)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);mixer=conversion_fixture::create(48000,2);FVM_AudioQueueBackend b=output_fixture::backend(device);assert(fvm_swf_sound_output_create(mixer,&b,4,8,&output)==FVM_OK);device.output=output;if(tracking)assert(fvm_swf_sound_output_set_identity_tracking(output,1)==FVM_OK);}
 ~FiniteOutput(){fvm_swf_sound_output_close(output);fvm_swf_sound_mixer_close(mixer);for(unsigned i=0;i<2;++i)fvm_swf_sound_voice_close(templates[i]);}
 void start(unsigned which=0,bool tracking=false){FVM_SWFSoundVoice *voice=0;assert(fvm_swf_sound_voice_clone(templates[which],(size_t)-1,&voice)==FVM_OK);assert((tracking?fvm_swf_sound_output_submit_prepared_bounded(output,&voice,(size_t)-1):fvm_swf_sound_mixer_submit_prepared(mixer,&voice))==FVM_OK&&!voice);}
};
#endif
