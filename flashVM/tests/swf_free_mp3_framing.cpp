/* Free-format framing against original encoder packets, with padding and immutable error output. */
#include "swf_free_mp3_fixture.hpp"
#include "../src/swf/sound_mp3.hpp"
#include <cassert>
#include <cstring>
int main()
{
 for(size_t i=0;i<sizeof(free_mp3_fixture::fixtures)/sizeof(free_mp3_fixture::fixtures[0]);++i){
  const free_mp3_fixture::Fixture &f=free_mp3_fixture::fixtures[i];
  FVM_SWFMP3Frame packet;
  assert(fvm_swf_mp3_probe(f.data,f.size,&packet)==FVM_OK);
  size_t base=(f.rate>=32000?144000UL:72000UL)*f.kbps/f.rate;
  assert(packet.bytes==base+((f.data[2]&2)!=0) && packet.rate==f.rate && packet.channels==f.channels && packet.samples==f.packet_samples);
  FVM_SWFMP3Frame saved=packet;
  assert(fvm_swf_mp3_probe(f.data,packet.bytes,&packet)==FVM_UNSUPPORTED && !std::memcmp(&saved,&packet,sizeof(packet)));
  unsigned char bad[4]={0,0,0,0};assert(fvm_swf_mp3_probe(bad,4,&packet)==FVM_INVALID_ARGUMENT && !std::memcmp(&saved,&packet,sizeof(packet)));
 }
 return 0;
}
