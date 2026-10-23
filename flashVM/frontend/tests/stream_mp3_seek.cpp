/* Real desktop queue, root navigation and signed MP3 starts in interpreter/JIT modes. */
#include "../common/audio.h"
#define main original_decode_main
#include "../../tests/swf_stream_mp3_decode.cpp"
#undef main
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/execution_mode.h"
#include <cstring>
/* Change the later block seek and optionally replace its preceding empty marker by a gap. */
static Bytes navigation_movie(unsigned latency,int adjustment,bool gap)
{
    Bytes swf=stream_movie(mp3_fixture::fixtures[0],latency);
    size_t at=14;unsigned blocks=0;
    while(at<swf.size())
    {
        size_t header_at=at;unsigned h=swf[at]|((unsigned)swf[at+1]<<8);at+=2;
        unsigned code=h>>6;size_t length=h&63;
        if(length==63){length=0;for(unsigned b=0;b<4;++b)length|=(size_t)swf[at+b]<<(8*b);at+=4;}
        if(code==19)
        {
            ++blocks;
            if(blocks==2 && gap){/* Preserve byte length using an authored frame label, leaving no audio block. */swf[header_at]=(unsigned char)((43<<6)|length);swf[header_at+1]=(unsigned char)(43>>2);swf[at]='g';swf[at+1]='a';swf[at+2]='p';swf[at+3]=0;}
            if(blocks==3){unsigned value=(unsigned)adjustment&65535;swf[at+2]=(unsigned char)value;swf[at+3]=(unsigned char)(value>>8);}
        }
        at+=length;
    }
    Bytes result,action;
    for(size_t i=0;i<14;++i)result.push_back(swf[i]);
    action.push_back(0x96);word(action,5);action.push_back(7);integer(action,1);action.push_back(0x17);action.push_back(0);
    tag(result,12,action);
    for(size_t i=14;i<swf.size();++i)result.push_back(swf[i]);
    for(unsigned i=0;i<4;++i)result[4+i]=(unsigned char)(result.size()>>(8*i));
    return result;
}
static void navigate(FVM_Context *c,FVM_Object *clip,const char *method,unsigned frame)
{FVM_Value argument,result;fvm_value_number(&argument,frame);assert(fvm_object_call(c,clip,method,1,&argument,&result)==FVM_OK);}
/* Owner operations run with a quiesced worker so cursor/PCM assertions are deterministic. */
static void verify(unsigned mode,unsigned latency,int adjustment,bool gap)
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *clip=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes swf=navigation_movie(latency,adjustment,gap);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_movieclip_load_swf_ex(c,swf.begin(),swf.size(),"_root",FVM_SWF_IMPORT_MUTE_ROOT_STREAMS,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);
    assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
    FVM_DesktopAudio audio;assert(fvm_desktop_audio_open_paused(c,clip,&audio,1)==FVM_OK);
    SDL_AtomicSet(&audio.running,0);SDL_WaitThread(audio.thread,0);audio.thread=0;
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    navigate(c,clip,"gotoAndPlay",gap?2:3);
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    FVM_SWFStreamSoundSeekPoint point;point.struct_size=sizeof(point);
    assert(fvm_swf_stream_sound_snapshot_seek_point(audio.stream_source,gap?2:3,&point)==FVM_OK);
    assert(audio.stream_output.converter->block_index==point.block_index && audio.stream_output.converter->first==point.first);
    size_t n=999;
    if(gap){assert(audio.stream_frame==0 && fvm_swf_stream_output_pump_until(&audio.stream_output,audio.stream_frame,&n)==FVM_NOT_FOUND && !n);assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);}
    assert(audio.stream_frame>=audio.stream_seek_pcm_frame && audio.stream_frame>=3);
    FVM_SWFStreamConverter expected=*audio.stream_output.converter;short samples[FVM_DESKTOP_AUDIO_BLOCK*2];FVM_SWFStreamConvertedChunk chunk;chunk.struct_size=sizeof(chunk);size_t written=0;
    assert(fvm_swf_stream_converter_next(&expected,FVM_DESKTOP_AUDIO_BLOCK,samples,FVM_DESKTOP_AUDIO_BLOCK*2,&chunk,&written)==FVM_OK && written);
    assert(fvm_swf_stream_output_pump_until(&audio.stream_output,audio.stream_frame,&n)==FVM_OK && n && !std::memcmp(samples,audio.stream_pcm,written*sizeof(short)));
    assert(fvm_gc_collect(c,0)==FVM_OK);navigate(c,clip,"gotoAndStop",1);
    assert(fvm_movieclip_advance(c,clip,1)==FVM_OK && fvm_desktop_audio_timeline(c,clip,&audio)==FVM_OK);
    assert(fvm_swf_stream_sound_snapshot_seek_point(audio.stream_source,1,&point)==FVM_OK && audio.stream_output.converter->block_index==point.block_index && audio.stream_output.converter->first==point.first && audio.stream_output.info.submitted_frames==0);
    FVM_ExecutionStats stats;stats.struct_size=sizeof(stats);assert(fvm_execution_get_stats(c,&stats)==FVM_OK);assert(mode?stats.native_entries>0:stats.interpreter_entries>0);
    fvm_desktop_audio_close(&audio);assert(fvm_gc_unpin(c,clip)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
int main(){for(unsigned mode=0;mode<2;++mode){if(mode && !fvm_jit_available())continue;verify(mode,37,37,false);verify(mode,1105,37,false);verify(mode,37,-74,true);}return 0;}
