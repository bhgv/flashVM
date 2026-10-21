/* Distinct-ID validation, maximum UI16 bitmap bit, stop-only/unreferenced sources and fresh retries. */
#include "../common/audio.h"
#include "../../tests/swf_sound_fixture.hpp"
#include "../../tests/swf_crc_mp3_fixture.hpp"
#include <cassert>
extern "C" void fvm_test_preflight_reset(void);
extern "C" unsigned int fvm_test_preflight_calls(void);
using namespace sound_fixture;
/* Build repeated events and an optional corrupt resource without invoking a codec to construct input. */
static Bytes source(bool bad, bool stop_only)
{
 const crc_mp3_fixture::Fixture &v=crc_mp3_fixture::fixtures[0];
 Bytes tags,payload;word(payload,0);for(size_t i=0;i<v.size;++i)payload.push_back(v.data[i]);
 tag(tags,14,sound(1,0x22,5760,payload.begin(),payload.size()));
 if(bad)payload[6]^=1;
 tag(tags,14,sound(65535,0x22,5760,payload.begin(),payload.size()));
 payload[6]^=1;
 tag(tags,14,sound(7,0x22,5760,payload.begin(),payload.size()));
 for(unsigned i=0;i<100;++i){Bytes event;word(event,1);event.push_back(0);tag(tags,15,event);}
 Bytes event;word(event,65535);event.push_back(stop_only?32:0);tag(tags,15,event);
 event.clear();word(event,7);event.push_back(32);tag(tags,15,event);
 return movie(tags);
}
/* No SDL endpoint is initialized: admission owns no device and executes before acquisition. */
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 for(unsigned pass=0;pass<3;++pass){
  Bytes wire=source(pass!=0,pass==2);assert(fvm_load_memory(c,wire.begin(),wire.size())==FVM_OK);
  for(unsigned repeat=0;repeat<2;++repeat){
   fvm_test_preflight_reset();assert(fvm_desktop_audio_validate(c)==(pass==1?FVM_INVALID_ARGUMENT:FVM_OK));
   assert(fvm_test_preflight_calls()==(pass==2?1U:2U));
  }
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
