/* Exact raw selectors, mixed flags and old source gates stay separate from global key events. */
#include "clip_keypress_fixture.hpp"
/* This native method must never be used as a fictitious onKeyPress notification. */
static FVM_Result forbidden(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{
    assert(false);return FVM_ERROR;
}
/* A selected subtree broadcasts retained KeyPress without state/focus/native method changes. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=clip_context(runtime);Bytes events;
        u16(events,0);clip_flags(events,version,version<6?64:0x20040);
        if(version<6)clip_record(events,version,64,action(9));
        else
        {
            clip_record(events,version,0x20000,action(11),1);
            clip_record(events,version,0x20040,action(22),'A');
            clip_record(events,version,0x20000,action(33),255);
            clip_record(events,version,0x20000,action(44),0);
        }
        clip_flags(events,version,0);
        FVM_Object *root=clip_load(c,clip_movie(version,events));
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK);
        FVM_Object *actor=get(c,root,"actor").object,*second=input_duplicate(c,actor,"second",3);
        assert(fvm_object_define_method_rooted(c,actor,"onKeyPress",forbidden,0,0)==FVM_OK);
        assert(fvm_register_function(c,"onKeyPress",forbidden,0)==FVM_OK);
        FVM_KeyboardState before=c->keyboard;
        const unsigned keys[]={1,'A',255,0};const unsigned marks[]={11,22,33,44};
        for(unsigned i=0;i<4;++i)
        {
            assert(fvm_movieclip_dispatch_key_press(c,root,keys[i])==FVM_OK);
            if(version>=6){number(c,actor,"seen",marks[i]);number(c,second,"seen",marks[i]);}
            else assert(get(c,actor,"seen").type==FVM_VALUE_UNDEFINED);
            assert(!std::memcmp(&before,&c->keyboard,sizeof(before)));
            input_clean(c);
        }
        assert(fvm_movieclip_dispatch_key_press(c,root,'B')==FVM_OK);
        if(version>=6)number(c,actor,"seen",44);
        assert(fvm_movieclip_dispatch_clip_input(c,root,64)==FVM_OK);
        number(c,actor,"seen",version<6?9:22);
        assert(fvm_movieclip_dispatch_key_press(c,root,256)==FVM_INVALID_ARGUMENT);
        assert(fvm_movieclip_dispatch_key_press(c,0,1)==FVM_INVALID_ARGUMENT);
        assert(fvm_movieclip_dispatch_key_press(0,root,1)==FVM_INVALID_ARGUMENT);
        input_clean(c);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
}
