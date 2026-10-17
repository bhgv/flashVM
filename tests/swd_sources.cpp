/* SWD6--8 association, source text, current positions and malformed-input rejection. */
#include "flashvm/source_debug.h"
#include "flashvm/crash.h"
#include "flashvm/debug.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
#include <zlib.h>
struct Bytes {
    fvm_detail::vector<unsigned char> data;
    void byte(unsigned n){data.push_back((unsigned char)n);}
    void number(unsigned long n,unsigned width=4){for(unsigned i=0;i<width;++i)byte((unsigned)(n>>(8*i)));}
    void text(const char *s){do{byte((unsigned char)*s);}while(*s++);}
};
static const unsigned char code[]={0x96,5,0,7,42,0,0,0,0x2a,0};
static const char source[]="// source\r\nthrow 42;\nvar other = 0;\n";
static unsigned paused_count;
static void check_position(FVM_Context *c)
{
    FVM_DebugSourcePosition p;assert(fvm_debug_current_source(c,&p)==FVM_OK);
    assert(p.module_id==7 && p.line==2 && p.source_size==7 && !std::memcmp(p.source,"Main.as",7));
    assert(p.line_size==9 && !std::memcmp(p.line_text,"throw 42;",9));
    assert(fvm_debug_clear_symbols(c)==FVM_INVALID_ARGUMENT);
}
static void breakpoint(FVM_Context *c,FVM_Result,void *)
{
    ++paused_count;check_position(c);
    const char *p;size_t n;assert(fvm_crash_source_line(c,0,&p,&n)==FVM_OK && n==9 && !std::memcmp(p,"throw 42;",9));
}
static FVM_Result stepped(FVM_Context *c,const FVM_DebugBreakpoint *point,void *)
{
    ++paused_count;check_position(c);
    size_t count;assert(fvm_debug_frame_count(c,&count)==FVM_OK && count==1);
    FVM_DebugFrame frame;assert(fvm_debug_frame_at(c,0,&frame)==FVM_OK && frame.engine==FVM_CRASH_ENGINE_INTERPRETER && frame.bytecode_size==sizeof(code));
    FVM_Value value;assert(fvm_crash_register(c,0,0,&value)==FVM_OK);
    assert(point->line==2 && !std::strcmp(point->source,"Main.as"));return FVM_OK;
}
static size_t movie(Bytes &swf)
{
    const unsigned char header[]={'F','W','S',8,0,0,0,0,8,0,0,12,1,0};
    for(size_t i=0;i<sizeof(header);++i)swf.byte(header[i]);
    swf.number((63<<6)|16,2);for(unsigned i=0;i<16;++i)swf.byte(i+1);
    swf.number((12<<6)|sizeof(code),2);size_t start=swf.data.size();
    for(size_t i=0;i<sizeof(code);++i)swf.byte(code[i]);
    swf.number(1<<6,2);swf.number(0,2);
    unsigned long length=(unsigned long)swf.data.size();for(unsigned i=0;i<4;++i)swf.data[4+i]=(unsigned char)(length>>(8*i));
    return start;
}
static void symbols(Bytes &swd,size_t start,bool offsets_first=false)
{
    swd.byte('F');swd.byte('W');swd.byte('D');swd.byte(8);
    swd.number(3);for(unsigned i=0;i<16;++i)swd.byte(i+1);
    if(offsets_first){swd.number(1);swd.number(7);swd.number(2);swd.number((unsigned long)start);}
    swd.number(0);swd.number(7);swd.number(2);swd.text("Main.as");swd.text(source);
    if(!offsets_first){swd.number(1);swd.number(7);swd.number(2);swd.number((unsigned long)start);}
    swd.number(5);swd.number((unsigned long)start);swd.byte(1);swd.byte(1);swd.text("saved");
    swd.number(2);swd.number((unsigned long)start);swd.number(0xffffffffUL);
}
#ifndef FVM_SWD_FIXTURE_ONLY
int main(int argc,char **argv)
{
    FVM_Config cfg;FVM_Runtime *rt;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    Bytes swf;size_t start=movie(swf);
    for(unsigned engine=0;engine<2;++engine)for(unsigned order=0;order<2;++order)
    {
        Bytes swd;symbols(swd,start,order!=0);FVM_Context *c;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size())==FVM_OK);
        size_t count;assert(fvm_debug_source_count(c,&count)==FVM_OK && count==1);
        unsigned long id;const char *name,*text;size_t name_size,text_size;
        assert(fvm_debug_source_at(c,0,&id,&name,&name_size,&text,&text_size)==FVM_OK && id==7 && text_size==sizeof(source)-1 && !std::memcmp(text,source,text_size));
        FVM_DebugSourcePosition p;assert(fvm_debug_source_position(c,code,sizeof(code),8,&p)==FVM_OK && p.line==2);
        assert(fvm_debug_register_name(c,code,sizeof(code),8,1,&name,&name_size)==FVM_OK && name_size==5 && !std::memcmp(name,"saved",5));
        assert(fvm_debug_register_name(c,code,sizeof(code),8,2,&name,&name_size)==FVM_NOT_FOUND);
        /* Every truncated prefix fails atomically, retaining the previous good bundle. */
        for(size_t n=0;n<swd.data.size();++n) {
            assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),n)!=FVM_OK);
            assert(fvm_debug_source_count(c,&count)==FVM_OK && count==1);
        }
        swd.data[8]^=1;assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size())==FVM_INVALID_ARGUMENT);swd.data[8]^=1;
        assert(fvm_execution_set_mode(c,engine?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        assert(fvm_crash_configure(c,FVM_CRASH_DEBUGGER,0,breakpoint,0)==FVM_OK);
        paused_count=0;assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_EXCEPTION && paused_count==1);
        FVM_CrashFrame frame;assert(fvm_crash_frame(c,0,&frame)==FVM_OK && frame.line==2 && frame.source_size==7);
        assert(fvm_exception_clear(c)==FVM_OK);
        /* Sources also resolve inside ordinary breakpoint callbacks with crash action NONE. */
        assert(fvm_crash_configure(c,FVM_CRASH_NONE,0,0,0)==FVM_OK);
        assert(fvm_debug_set_breakpoint(c,"",8,1)==FVM_OK && fvm_debug_set_breakpoint_handler(c,stepped,0)==FVM_OK);
        paused_count=0;assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_EXCEPTION && paused_count==1);
        assert(fvm_exception_clear(c)==FVM_OK && !c->crash.active);
        swd.data[4]=4;assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size())==FVM_UNSUPPORTED);swd.data[4]=3;
        swd.data[3]=9;assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size())==FVM_UNSUPPORTED);swd.data[3]=8;
        /* Matching compressed CWS uses uncompressed absolute SWD offsets. */
        Bytes compressed;for(size_t i=0;i<8;++i)compressed.byte(swf.data[i]);compressed.data[0]='C';
        uLongf bound=compressBound((uLong)(swf.data.size()-8));fvm_detail::vector<unsigned char> zipped((size_t)bound);
        assert(compress2(zipped.begin(),&bound,swf.data.begin()+8,(uLong)(swf.data.size()-8),9)==Z_OK);
        for(size_t i=0;i<(size_t)bound;++i)compressed.byte(zipped[i]);
        assert(fvm_debug_load_swd(c,compressed.data.begin(),compressed.data.size(),swd.data.begin(),swd.data.size())==FVM_OK);
        assert(fvm_debug_clear_symbols(c)==FVM_OK && fvm_debug_source_count(c,&count)==FVM_OK && !count);
        /* Already retained source line bytes survive clearing all imported symbols. */
        assert(fvm_crash_source_line(c,0,&name,&name_size)==FVM_OK && name_size==9);
        fvm_context_destroy(c);
    }
    {
        FVM_Context *c;assert(fvm_context_create(rt,&c)==FVM_OK);
        Bytes duplicate;movie(duplicate);duplicate.data.resize(duplicate.data.size()-4);
        duplicate.number((12<<6)|sizeof(code),2);size_t second=duplicate.data.size();
        for(size_t i=0;i<sizeof(code);++i)duplicate.byte(code[i]);duplicate.number(1<<6,2);duplicate.number(0,2);
        unsigned long total=(unsigned long)duplicate.data.size();for(unsigned i=0;i<4;++i)duplicate.data[4+i]=(unsigned char)(total>>(8*i));
        Bytes swd;symbols(swd,start);swd.data.resize(swd.data.size()-4);
        swd.number(0);swd.number(8);swd.number(2);swd.text("Other.as");swd.text("other source line");
        swd.number(1);swd.number(8);swd.number(1);swd.number((unsigned long)second);swd.number(0xffffffffUL);
        assert(fvm_debug_load_swd(c,duplicate.data.begin(),duplicate.data.size(),swd.data.begin(),swd.data.size())==FVM_OK);
        FVM_DebugSourcePosition p;assert(fvm_debug_source_position(c,code,sizeof(code),8,&p)==FVM_NOT_FOUND);
        fvm_context_destroy(c);
    }
    if(argc==2)
    {
        FVM_Context *c;assert(fvm_context_create(rt,&c)==FVM_OK);
        Bytes swd;symbols(swd,start);
        assert(fvm_debug_load_swd(c,swf.data.begin(),swf.data.size(),swd.data.begin(),swd.data.size())==FVM_OK);
        assert(fvm_crash_configure(c,FVM_CRASH_ZIP,argv[1],0,0)==FVM_OK);
        assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_EXCEPTION);
        size_t frames;FVM_Result status,diagnostic;assert(fvm_crash_info(c,&frames,&status,&diagnostic)==FVM_OK && diagnostic==FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}

#endif
