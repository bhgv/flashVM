/* Loaded canonical root resources, endian boundaries, CWS and callback reload safety. */
#include "flashvm/swf_sound.h"
#include "flashvm/avm1.h"
#include "flashvm/execution_mode.h"
#include "swf_sound_fixture.hpp"
#include "zlib.h"
using namespace sound_fixture;
struct Sink { FVM_Context *context;const Bytes *reload;short values[8];size_t count;unsigned writes,stops;double seconds; };
static FVM_Result write(void *raw,const FVM_MediaFrame *frame)
{Sink *sink=(Sink *)raw;++sink->writes;sink->seconds=frame->seconds;assert(frame->has_timestamp && frame->size<=sizeof(sink->values));if(sink->reload)assert(fvm_load_memory(sink->context,sink->reload->begin(),sink->reload->size())==FVM_OK);sink->count=frame->size/sizeof(short);std::memcpy(sink->values,frame->data,frame->size);return FVM_OK;}
static FVM_Result stop(void *raw){++((Sink *)raw)->stops;return FVM_OK;}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 const unsigned char data[]={0,128,255,127,255,255,0,0,1,0,254,255};
 for(unsigned version=1;version<=8;++version)for(unsigned stereo=0;stereo<2;++stereo)for(unsigned mode=0;mode<2;++mode)
 {
 FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
 unsigned flags=(version<4?0:0x30)|0xe|stereo;Bytes definitions,payload=sound(9,flags,stereo?3:6,data,sizeof(data));tag(definitions,14,payload);Bytes swf=movie(definitions,version);
 assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);FVM_SWFSoundInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_info(c,9,&info)==FVM_OK && info.sample_count==(stereo?3UL:6UL));
 const unsigned char *stable=info.data;const unsigned char bad_file[]={'F','W'};assert(fvm_load_memory(c,bad_file,sizeof(bad_file))==FVM_INVALID_ARGUMENT);assert(fvm_swf_sound_info(c,9,&info)==FVM_OK && info.data==stable);
 short pcm[8]={0};size_t count=0;assert(fvm_swf_sound_read_pcm(c,9,0,info.sample_count,pcm,8,&count)==FVM_OK && count==6);
 if(version>=4){assert(pcm[0]==-32768 && pcm[1]==32767 && pcm[2]==-1 && pcm[3]==0 && pcm[4]==1 && pcm[5]==-2);}
 assert(fvm_swf_sound_read_pcm(c,9,1,1,pcm,8,&count)==FVM_OK && count==(stereo?2U:1U));
 Sink sink;std::memset(&sink,0,sizeof(sink));sink.context=c;FVM_MediaOutput output;output.struct_size=sizeof(output);output.userdata=&sink;output.write=write;output.stop=stop;
 FVM_AudioChannel *channel=0,*foreign=0;assert(fvm_audio_channel_create(c,&output,&channel)==FVM_OK && fvm_audio_channel_create(other,&output,&foreign)==FVM_OK);
 assert(fvm_swf_sound_submit(c,9,foreign,0,1)==FVM_INVALID_ARGUMENT && !sink.writes);
 assert(fvm_swf_sound_submit(c,9,channel,info.sample_count,0)==FVM_OK && !sink.writes);
 Bytes empty,replacement=movie(empty,version);sink.reload=&replacement;
 assert(fvm_swf_sound_submit(c,9,channel,1,1)==FVM_OK && sink.writes==1 && sink.count==(stereo?2U:1U) && sink.seconds==1.0/44100);
 assert(!std::memcmp(sink.values,pcm,sink.count*sizeof(short)));
 assert(fvm_swf_sound_info(c,9,&info)==FVM_NOT_FOUND);const unsigned char halt[]={9,0};assert(fvm_avm1_execute(c,halt,sizeof(halt))==FVM_OK && sink.stops==1);
 assert(fvm_audio_channel_close(channel)==FVM_OK && fvm_audio_channel_close(foreign)==FVM_OK);
 /* CWS canonical ownership does not borrow the compressed input storage. */
 if(version>=6){Bytes compressed;compressed.resize(8+compressBound((uLong)(swf.size()-8)));std::memcpy(compressed.begin(),swf.begin(),8);compressed[0]='C';uLongf size=compressed.size()-8;assert(compress2(compressed.begin()+8,&size,swf.begin()+8,(uLong)(swf.size()-8),Z_BEST_COMPRESSION)==Z_OK);compressed.resize(8+size);assert(fvm_load_memory(c,compressed.begin(),compressed.size())==FVM_OK);compressed.clear();assert(fvm_swf_sound_info(c,9,&info)==FVM_OK && info.data_size==sizeof(data));}
 /* Metadata admits opaque codecs; eight-bit PCM decodes to signed-16. */
 definitions.clear();payload=sound(9,0x20,17,data,3);tag(definitions,14,payload);swf=movie(definitions,version);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);assert(fvm_swf_sound_info(c,9,&info)==FVM_OK && info.data_size==3);assert(fvm_swf_sound_read_pcm(c,9,0,0,0,0,&count)==FVM_UNSUPPORTED && !count);
 definitions.clear();payload=sound(9,0x0c,3,data,3);tag(definitions,14,payload);swf=movie(definitions,version);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);assert(fvm_swf_sound_info(c,9,&info)==FVM_OK && info.bits_per_sample==8);assert(fvm_swf_sound_read_pcm(c,9,0,1,pcm,8,&count)==FVM_OK && count==1 && pcm[0]==-32768);
 /* Declared PCM size mismatch, zero ID, short headers and selected duplicate IDs are rejected. */
 for(unsigned bad=0;bad<4;++bad){definitions.clear();payload=sound(bad==1?0:9,0x0e,bad==0?99:6,data,sizeof(data));if(bad==2)payload.resize(6);tag(definitions,14,payload);if(bad==3)tag(definitions,14,payload);swf=movie(definitions,version);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);info.sound_id=77;assert(fvm_swf_sound_info(c,9,&info)==FVM_INVALID_ARGUMENT && info.sound_id==77);}
 fvm_context_destroy(other);fvm_context_destroy(c);
 }
 {FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);Bytes definitions,big;for(unsigned i=0;i<80;++i)big.push_back((unsigned char)i);tag(definitions,14,sound(5,0x3e,40,big.begin(),big.size()));tag(definitions,14,sound(7,0x3e,6,data,sizeof(data)));Bytes swf=movie(definitions);assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);size_t count=0;FVM_SWFSoundInfo info;info.struct_size=sizeof(info);assert(fvm_swf_sound_count(c,&count)==FVM_OK && count==2);assert(fvm_swf_sound_at(c,0,&info)==FVM_OK && info.sound_id==5 && info.data_size==80);assert(fvm_swf_sound_at(c,1,&info)==FVM_OK && info.sound_id==7);assert(fvm_swf_sound_at(c,2,&info)==FVM_NOT_FOUND && info.sound_id==7);fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
