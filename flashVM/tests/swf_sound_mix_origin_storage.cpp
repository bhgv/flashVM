/* Origin predictor accounts exact optional bitmap bytes and preserves output on invalid modes. */
#include "swf_stream_mix_gate_fixture.hpp"
int main()
{
 size_t base=0,ids=0,origins=0,both=0;
 assert(fvm_swf_sound_mix_queue_storage_required(2,2,&base)==FVM_OK);
 assert(fvm_swf_sound_mix_queue_storage_required_for_tracking(2,2,1,&ids)==FVM_OK);
 assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,0,1,&origins)==FVM_OK&&origins-base==32768);
 assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,1,1,&both)==FVM_OK&&both-ids==32768);
 size_t old=both;assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,1,2,&both)==FVM_INVALID_ARGUMENT&&both==old);
 assert(fvm_swf_sound_mix_queue_storage_required_for_modes(2,2,2,1,&both)==FVM_INVALID_ARGUMENT&&both==old);
 GateFixture f;FVM_SWFSoundMixQueueStorageInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_mix_queue_storage_info(f.mix,&info)==FVM_OK&&info.retained_bytes==ids);return 0;
}
