/* Native Call retains parent operands through nested frame execution and target restoration. */
#define FVM_DISPATCH_FIXTURE_ONLY
#include "jit_dispatch.cpp"
#include "flashvm/movieclip.h"
#include "flashvm/debug.h"
#include "../src/avm1/block.hpp"
static FVM_Value frame_dispatch_run(FVM_Context *c,Program p,FVM_Object *receiver,unsigned nested=1,FVM_Result expected=FVM_OK,bool native_frames=true)
{
    p.finish();FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;fvm_value_undefined(&out);
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
    assert(fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
    if(class_mode==1)assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
    unsigned long entries=c->execution_mode.stats.native_entries,interpreted=c->execution_mode.stats.interpreter_entries;FVM_GCRootFrame *roots=c->gc_frames;
    assert(fvm_function_invoke(c,fn,receiver,0,0,&out)==expected);
    assert(c->execution_mode.stats.native_entries==entries+(class_mode==0?0:1+(class_mode==2 && native_frames?nested:0)));
    assert(c->execution_mode.stats.interpreter_entries==interpreted+(class_mode==0?1:0)+(class_mode==2 && native_frames?0:nested));
    assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth && !code->active);
    if(expected==FVM_OK)assert(out.type==FVM_VALUE_NUMBER && out.number==77);
    if(expected==FVM_EXCEPTION)assert(fvm_exception_clear(c)==FVM_OK);
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);return out;
}
static void frame_define(FVM_Context *c,FVM_Object *clip,unsigned frame,const char *label,unsigned value)
{Program body;body.text("seen");body.number(value);body.op(0x1d);body.op(0);assert(fvm_movieclip_define_frame(c,clip,frame,label,body.bytes.begin(),body.bytes.size())==FVM_OK);}
static void frame_seen(FVM_Context *c,FVM_Object *clip,unsigned expected)
{FVM_Value value;assert(fvm_object_get(c,clip,"seen",&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==expected);assert(fvm_object_get(c,clip,"_currentframe",&value)==FVM_OK && value.number==1);}
static unsigned frame_breaks;
static FVM_Result frame_break(FVM_Context *,const FVM_DebugBreakpoint *point,void *)
{assert(!std::strcmp(point->function,"[Frame Call]") && !point->offset);++frame_breaks;return FVM_OK;}
#ifndef FVM_FRAME_DISPATCH_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        FVM_Object *root=0,*child=0;FVM_Value args[2],out;
        assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",3,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        fvm_value_string(args,"child");fvm_value_number(args+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);child=out.object;
        frame_define(c,root,2,"second",42);frame_define(c,root,3,"third",43);frame_define(c,child,1,"first",99);
        Program number;number.number(77);number.number(2);dispatch_call(number);frame_dispatch_run(c,number,root);frame_seen(c,root,42);
        Program label;label.number(77);label.text("third");dispatch_call(label);frame_dispatch_run(c,label,root);frame_seen(c,root,43);
        Program path;path.number(77);path.text("child:first");dispatch_call(path);frame_dispatch_run(c,path,root);frame_seen(c,child,99);frame_seen(c,root,43);
        Program selected;selected.number(77);selected.text("child");selected.op(0x20);selected.text("first");dispatch_call(selected);frame_dispatch_run(c,selected,root);frame_seen(c,child,99);
        Program recovery;recovery.number(77);recovery.text("missing");recovery.op(0x20);recovery.text("second");dispatch_call(recovery);frame_dispatch_run(c,recovery,root);frame_seen(c,root,42);
        assert(fvm_value_string_bytes(&out,"c\0x",3)==FVM_OK && fvm_object_set(c,child,"_name",&out)==FVM_OK);
        assert(fvm_value_string_bytes(&out,"c\0x:1",5)==FVM_OK && fvm_set_global(c,"request",&out)==FVM_OK);
        Program counted;counted.number(77);counted.variable("request");dispatch_call(counted);frame_dispatch_run(c,counted,root);frame_seen(c,child,99);
        Program absent;absent.number(77);absent.text("missing");dispatch_call(absent);frame_dispatch_run(c,absent,root,0);
        Program zero;zero.number(77);zero.number(0);dispatch_call(zero);frame_dispatch_run(c,zero,root,0);
        Program empty;dispatch_call(empty);empty.number(77);frame_dispatch_run(c,empty,root,0,version?FVM_OK:FVM_ERROR);
        Program raising;raising.text("boom");raising.op(0x2a);raising.op(0);assert(fvm_movieclip_define_frame(c,root,3,"third",raising.bytes.begin(),raising.bytes.size())==FVM_OK);
        frame_dispatch_run(c,label,root,1,FVM_EXCEPTION);frame_define(c,root,3,"third",43);frame_dispatch_run(c,label,root);frame_seen(c,root,43);
        Program recursive;recursive.number(2);dispatch_call(recursive);recursive.op(0);
        assert(fvm_movieclip_define_frame(c,root,2,"second",recursive.bytes.begin(),recursive.bytes.size())==FVM_OK);
        frame_dispatch_run(c,number,root,63,FVM_ERROR);
        for(fvm_detail::map<fvm_detail::string,FVM_JitCode *>::iterator cached=c->execution_mode.cache.begin();cached!=c->execution_mode.cache.end();++cached)assert(!cached->second || !cached->second->active);
        frame_define(c,root,2,"second",42);frame_dispatch_run(c,number,root);frame_seen(c,root,42);
        const unsigned char fallback[]={0x99,2,0,1,0,0x08,0};
        assert(fvm_movieclip_define_frame(c,root,2,"second",fallback,sizeof(fallback))==FVM_OK);
        frame_dispatch_run(c,number,root,1,FVM_OK,false);frame_define(c,root,2,"second",42);
        frame_breaks=0;assert(fvm_debug_set_breakpoint_handler(c,frame_break,0)==FVM_OK && fvm_debug_set_breakpoint(c,"[Frame Call]",0,1)==FVM_OK);
        frame_dispatch_run(c,number,root,1,FVM_OK,false);assert(frame_breaks==1);
        assert(fvm_debug_set_breakpoint(c,"[Frame Call]",0,0)==FVM_OK && fvm_debug_set_breakpoint_handler(c,0,0)==FVM_OK);
        frame_dispatch_run(c,number,root);frame_seen(c,root,42);
        if(version)
        {
            Program fresh;fresh.text("seen");fresh.op(0x96);fresh.op(2);fresh.op(0);fresh.op(8);fresh.op(0);fresh.op(0x1d);fresh.op(0);
            assert(fvm_movieclip_define_frame(c,root,2,"second",fresh.bytes.begin(),fresh.bytes.size())==FVM_OK);
            Program pool;pool.op(0x88);pool.op(9);pool.op(0);pool.op(1);pool.op(0);
            const char *text="parent";for(unsigned i=0;i<7;++i)pool.op((unsigned char)text[i]);
            pool.number(77);pool.number(2);dispatch_call(pool);pool.text("parentAfter");pool.op(0x96);pool.op(2);pool.op(0);pool.op(8);pool.op(0);pool.op(0x1d);
            frame_dispatch_run(c,pool,root);assert(fvm_object_get(c,root,"seen",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            assert(fvm_object_get(c,root,"parentAfter",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"parent"));
        }
        if(class_mode!=1)
        {
            Program bounded;bounded.text("seen");bounded.number(44);bounded.op(0x1d);
            assert(fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
            unsigned long entries=c->execution_mode.stats.native_entries;
            assert(fvm_avm1_execute_block_receiver(c,bounded.bytes.begin(),bounded.bytes.size(),root)==FVM_OK);
            assert(c->execution_mode.stats.native_entries==entries+(class_mode==2));frame_seen(c,root,44);
            assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
        }
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    for(unsigned limit=7;limit<=8;++limit)
    {
        cfg.instruction_limit=limit;assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
        for(unsigned loaded=0;loaded<2;++loaded)for(class_mode=0;class_mode<3;++class_mode)
        {
            FVM_Context *c=0;FVM_Object *root=0;const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
            assert(fvm_context_create(rt,&c)==FVM_OK);if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
            assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",2,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);frame_define(c,root,2,"second",42);
            Program p;p.number(77);p.number(2);dispatch_call(p);frame_dispatch_run(c,p,root,1,limit==8?FVM_OK:FVM_ERROR);frame_seen(c,root,42);fvm_context_destroy(c);
        }
        fvm_runtime_destroy(rt);
    }
    return 0;
}
#endif
