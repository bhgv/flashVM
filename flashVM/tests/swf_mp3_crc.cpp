/* Encoder-produced MPEG-1/2/2.5 mono/stereo CRCs, protected bit flips and main-data scope. */
#include "swf_crc_mp3_fixture.hpp"
#include "swf_mp3_fixture.hpp"
#include "../src/swf/sound_mp3.hpp"
#include <cassert>
#include <cstring>
int main()
{
 for(size_t f=0;f<sizeof(crc_mp3_fixture::fixtures)/sizeof(crc_mp3_fixture::fixtures[0]);++f){
  const crc_mp3_fixture::Fixture &v=crc_mp3_fixture::fixtures[f];
  size_t base=(v.rate>=32000?144000UL:72000UL)*v.kbps/v.rate;
  size_t at=0;
  while(at<v.size){
   size_t size=base+((v.data[at+2]&2)!=0);assert(size<=v.size-at && size<2304);
   assert(!(v.data[at+1]&1) && fvm_swf_mp3_verify_crc(v.data+at,size)==FVM_OK);
   unsigned char copy[2304];std::memcpy(copy,v.data+at,size);
   size_t side=v.packet_samples==1152?(v.channels==1?17:32):(v.channels==1?9:17);
   for(size_t byte=2;byte<side+6;++byte)for(unsigned bit=0;bit<8;++bit){
    copy[byte]^=(unsigned char)(1U<<bit);
    assert(fvm_swf_mp3_verify_crc(copy,size)!=FVM_OK);
    copy[byte]^=(unsigned char)(1U<<bit);
   }
   copy[size-1]^=1;assert(fvm_swf_mp3_verify_crc(copy,size)==FVM_OK);
   for(size_t n=0;n<side+6;++n)assert(fvm_swf_mp3_verify_crc(v.data+at,n)!=FVM_OK);
   at+=size;
  }
  assert(at==v.size);
 }
 FVM_SWFMP3Frame packet;
 assert(fvm_swf_mp3_probe(mp3_fixture::fixtures[0].data,mp3_fixture::fixtures[0].size,&packet)==FVM_OK);
 assert(fvm_swf_mp3_verify_crc(mp3_fixture::fixtures[0].data,packet.bytes)==FVM_OK);
 assert(fvm_swf_mp3_verify_crc(0,0)==FVM_INVALID_ARGUMENT);
 return 0;
}
