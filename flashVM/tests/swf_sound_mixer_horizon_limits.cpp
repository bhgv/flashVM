/* White-box repeat clamping proves bounded arithmetic without allocating an enormous source. */
#include "../src/swf/sound_mixer.cpp"
#include <cassert>
int main()
{
 FVM_SWFSoundVoiceInfo info;std::memset(&info,0,sizeof(info));info.struct_size=sizeof(info);info.frames_per_play=(size_t)-1;info.frame_in_play=(size_t)-2;info.play_count=UINT_MAX;
 assert(source_horizon(info,65536)==65536);
 info.play_count=1;assert(source_horizon(info,65536)==1);
 info.frames_per_play=1;info.frame_in_play=0;info.play_count=UINT_MAX;assert(source_horizon(info,65536)==65536);
 info.play_index=UINT_MAX-2;assert(source_horizon(info,65536)==2);
 info.finished=1;assert(!source_horizon(info,65536));size_t n=91;assert(fvm_swf_sound_mixer_active_frames(0,1,&n)==FVM_INVALID_ARGUMENT && n==91);
 return 0;
}
