/* Independent duration/PCM matrix checks every live fractional phase, channel mapping and repeat. */
#include "swf_sound_conversion_fixture.hpp"
using namespace conversion_fixture;
int main()
{
 const unsigned outputs[]={5512,8000,11025,44100,48000,192000};const unsigned plays[]={0,1,3};const size_t caps[]={1,2,7,FVM_SWF_SOUND_MIXER_MAX_HORIZON};
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 for(unsigned ri=0;ri<4;++ri)for(unsigned oi=0;oi<6;++oi)for(unsigned sc=1;sc<=2;++sc)for(unsigned oc=1;oc<=2;++oc)for(unsigned pi=0;pi<3;++pi)for(unsigned length=1;length<=7;length+=2)for(unsigned skip=0;skip<2;++skip)
 {
  if(skip && (length==1||!plays[pi]))continue;
  Samples samples;for(unsigned i=0;i<length*sc;++i)samples.push_back((short)((i%3?12345:-23456)+i*37));
  Bytes swf=movie(samples,ri,sc,plays[pi]);FVM_SWFSoundVoice *voice=0;
  assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&voice)==FVM_OK);
  if(skip)assert(fvm_swf_sound_voice_seek(voice,0,1)==FVM_OK);
  FVM_SWFSoundMixer *mixer=create(outputs[oi],oc,1);assert(fvm_swf_sound_mixer_submit_prepared(mixer,&voice)==FVM_OK && !voice);
  size_t total=(size_t)std::ceil((double)(length*plays[pi]-skip)*outputs[oi]/rates[ri]);
  for(size_t frame=0;frame<=total;++frame)
  {
   for(unsigned ci=0;ci<4;++ci){size_t n=91;size_t expected=total-frame<caps[ci]?total-frame:caps[ci];assert(fvm_swf_sound_mixer_active_frames(mixer,caps[ci],&n)==FVM_OK && n==expected);}
   if(frame==total)break;
   short pcm[2];size_t written=0;assert(fvm_swf_sound_mixer_read(mixer,1,pcm,2,&written)==FVM_OK && written==oc);
   if(!skip)for(unsigned channel=0;channel<oc;++channel)assert(pcm[channel]==oracle(samples,rates[ri],sc,outputs[oi],oc,frame,channel,plays[pi]));
  }
  assert(!active(mixer));fvm_swf_sound_mixer_close(mixer);
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
