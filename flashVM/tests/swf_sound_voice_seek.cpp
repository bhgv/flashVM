/* All native positions reconstruct elapsed envelope time; seeks never allocate or use the VM. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define main styled_reference_main
#include "swf_sound_voice.cpp"
#undef main
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes data;for(unsigned i=0;i<5;++i)word(data,(unsigned short)(2000+i*500));
    Bytes swf=file(0x3e,5,data,8);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK);
    FVM_SWFSoundVoice *v=0,*empty=0,*zero=0;
    assert(fvm_swf_sound_voice_open(c,0,&v)==FVM_OK);
    Bytes tags,e;tag(tags,14,sound(7,0x3e,0,0,0));word(e,7);e.push_back(0);tag(tags,15,e);swf=movie(tags);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&empty)==FVM_OK);
    tags.clear();e.clear();tag(tags,14,sound(7,0x3e,5,data.begin(),data.size()));word(e,7);e.push_back(4);word(e,0);tag(tags,15,e);swf=movie(tags);
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&zero)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    short golden[15],out[15];size_t n=0;
    assert(fvm_swf_sound_voice_read(v,15,golden,15,&n)==FVM_OK && n==15);
    allowance=0;
    for(unsigned at=15;at;--at)
    {
        unsigned position=at-1;
        assert(fvm_swf_sound_voice_seek(v,position/5,position%5)==FVM_OK);
        assert(fvm_swf_sound_voice_read(v,15,out,15,&n)==FVM_OK && n==15-position);
        assert(!std::memcmp(out,golden+position,n*sizeof(short)));
    }
    assert(fvm_swf_sound_voice_seek(v,0,0)==FVM_OK);
    assert(fvm_swf_sound_voice_seek(v,3,1)==FVM_INVALID_ARGUMENT && fvm_swf_sound_voice_seek(v,4,0)==FVM_INVALID_ARGUMENT && fvm_swf_sound_voice_seek(v,0,5)==FVM_INVALID_ARGUMENT);
    FVM_SWFSoundVoiceInfo info;info.struct_size=sizeof(info);
    assert(fvm_swf_sound_voice_info(v,&info)==FVM_OK && !info.play_index && !info.frame_in_play && !info.finished);
    assert(fvm_swf_sound_voice_seek(v,3,0)==FVM_OK && fvm_swf_sound_voice_read(v,1,out,15,&n)==FVM_NOT_FOUND && !n);
    assert(fvm_swf_sound_voice_seek(0,0,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_voice_seek(empty,0,0)==FVM_OK && fvm_swf_sound_voice_seek(empty,1,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_voice_seek(zero,0,0)==FVM_OK && fvm_swf_sound_voice_seek(zero,0,1)==FVM_INVALID_ARGUMENT);
    fvm_swf_sound_voice_close(v);fvm_swf_sound_voice_close(empty);fvm_swf_sound_voice_close(zero);
    assert(allowance==0);allowance=-1;return 0;
}
