/* Count/length-derived free-format inference, grouped packets, padding and atomic bad metadata. */
#include "swf_free_mp3_fixture.hpp"
#include "../src/swf/sound_mp3.hpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
int main()
{
 for(size_t f=0;f<sizeof(free_mp3_fixture::fixtures)/sizeof(free_mp3_fixture::fixtures[0]);++f){
  const free_mp3_fixture::Fixture &v=free_mp3_fixture::fixtures[f];
  size_t base=(v.rate>=32000?144000UL:72000UL)*v.kbps/v.rate;
  for(unsigned group=1;group<=5;++group){
   FVM_SWFMP3Decoder decoder;size_t at=0,total=0;
   while(at<v.size){
    size_t end=at;unsigned packets=0;
    while(end<v.size && packets<group){end+=base+((v.data[end+2]&2)!=0);++packets;}
    assert(end<=v.size);
    assert(decoder.prepare_block(v.data+at,end-at,packets*v.packet_samples)==FVM_OK);
    while(at<end){FVM_SWFMP3Frame info;assert(decoder.decode(v.data+at,end-at,&info)==FVM_OK);
     for(size_t p=0;p<sizeof(free_mp3_fixture::positions)/sizeof(free_mp3_fixture::positions[0]);++p)
      if(free_mp3_fixture::positions[p]>=total && free_mp3_fixture::positions[p]-total<info.samples)
       for(unsigned ch=0;ch<v.channels;++ch)
        assert(std::labs((long)decoder.pcm[(free_mp3_fixture::positions[p]-total)*v.channels+ch]-v.golden[p*v.channels+ch])<=2);
     at+=info.bytes;total+=info.samples;
    }
   }
   assert(total==5760);
  }
  FVM_SWFMP3Decoder decoder;mp3dec_t saved=decoder.state;
  assert(decoder.prepare_block(v.data,v.size,1)==FVM_INVALID_ARGUMENT && !std::memcmp(&saved,&decoder.state,sizeof(saved)));
  assert(decoder.prepare_block(v.data,v.size,0)==FVM_INVALID_ARGUMENT && !std::memcmp(&saved,&decoder.state,sizeof(saved)));
  assert(decoder.prepare_block(v.data,v.size,65536U)==FVM_INVALID_ARGUMENT && !std::memcmp(&saved,&decoder.state,sizeof(saved)));
  assert(decoder.prepare_block(v.data,v.size-1,5760)==FVM_INVALID_ARGUMENT && !std::memcmp(&saved,&decoder.state,sizeof(saved)));
  assert(decoder.prepare_block(0,0,0)==FVM_OK && !std::memcmp(&saved,&decoder.state,sizeof(saved)));
  assert(decoder.prepare_block(v.data,v.size,5760)==FVM_OK);
 }
 return 0;
}
