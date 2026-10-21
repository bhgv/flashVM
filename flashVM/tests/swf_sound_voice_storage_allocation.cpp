/* Isolated allocation boundaries preserve templates and release every partially copied owner. */
#define main allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define main styled_reference_main
#include "swf_sound_voice.cpp"
#undef main
#include "flashvm/swf_sound_mixer.h"
int main(int argc,char **argv)
{
    long first=0,last=4;int mode=0;
    if(argc==3)
    {
        mode=!std::strcmp(argv[1],"--clone-budget")?1:!std::strcmp(argv[1],"--open-budget")?2:!std::strcmp(argv[1],"--mixer-budget")?3:0;
        assert(mode);char *end=0;first=std::strtol(argv[2],&end,10);assert(end && !*end && first>=0 && first<=3);last=first+1;
    }
    else assert(argc==1);
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    Bytes data;for(unsigned i=0;i<5;++i)word(data,(unsigned short)(2000+i*500));
    Bytes swf=file(0x3e,5,data,8);FVM_SWFSoundVoice *source=0;
    assert(fvm_load_memory(c,swf.begin(),swf.size())==FVM_OK && fvm_swf_sound_voice_open(c,0,&source)==FVM_OK);
    FVM_SWFSoundVoiceStorageInfo storage;storage.struct_size=sizeof(storage);
    assert(fvm_swf_sound_voice_storage_info(source,&storage)==FVM_OK && fvm_swf_sound_voice_seek(source,1,2)==FVM_OK);
    for(int operation=1;operation<=3;++operation)
        if(!mode || mode==operation)
            for(long budget=first;budget<last;++budget)
            {
                FVM_SWFSoundVoice *voice=(FVM_SWFSoundVoice*)1;
                FVM_SWFSoundMixer *mixer=(FVM_SWFSoundMixer*)1;
                FVM_SWFSoundMixerConfig geometry={sizeof(geometry),44100,1,2,0};
                allowance=budget;
                FVM_Result result=operation==1?fvm_swf_sound_voice_clone(source,storage.retained_bytes,&voice):operation==2?fvm_swf_sound_voice_open_bounded(c,0,storage.retained_bytes,&voice):fvm_swf_sound_mixer_create_bounded(&geometry,(size_t)-1,&mixer);
                allowance=-1;
                unsigned required=operation==3?2:3;
                if((unsigned long)budget<required)
                {
                    assert(result==FVM_OUT_OF_MEMORY);
                    assert(operation==3?!mixer:!voice);
                }
                else
                {
                    assert(result==FVM_OK);
                    if(operation==3)fvm_swf_sound_mixer_close(mixer);
                    else
                    {
                        FVM_SWFSoundVoiceInfo info;info.struct_size=sizeof(info);
                        assert(fvm_swf_sound_voice_info(voice,&info)==FVM_OK && info.play_index==(operation==1?1U:0U) && info.frame_in_play==(operation==1?2U:0U));
                        fvm_swf_sound_voice_close(voice);
                    }
                }
                FVM_SWFSoundVoiceInfo info;info.struct_size=sizeof(info);
                assert(fvm_swf_sound_voice_info(source,&info)==FVM_OK && info.play_index==1 && info.frame_in_play==2);
                assert(fvm_gc_collect(c,0)==FVM_OK);
            }
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    short pcm[16];size_t n=0;allowance=0;
    assert(fvm_swf_sound_voice_read(source,16,pcm,16,&n)==FVM_OK && n==8);
    fvm_swf_sound_voice_close(source);assert(allowance==0);allowance=-1;return 0;
}
