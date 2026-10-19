/* Independent URL spelling fixtures, primitive coercion, version gates and counted UTF-16 roundtrips. */
#include "../src/core/runtime.hpp"
#include "../src/core/execution_work_scope.hpp"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <cassert>
#include <cstring>
static void same(const FVM_Value &value, const char *bytes, size_t size)
{ assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == size && !std::memcmp(value.string, bytes, size)); }
static FVM_Value invoke(FVM_Context *c, const char *name, const FVM_Value *args, unsigned count, bool extracted, FVM_Result expected = FVM_OK)
{
    FVM_Value out;
    if (!extracted) assert(fvm_call(c, name, count, args, &out) == expected);
    else { FVM_Value fn; assert(fvm_get_global(c, name, &fn) == FVM_OK); assert(fvm_function_invoke(c, fn.object, 0, count, args, &out) == expected); }
    if (expected != FVM_OK) assert(out.type == FVM_VALUE_UNDEFINED);
    return out;
}
struct State { unsigned calls; FVM_Result result; };
static FVM_Result text(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    State *s = (State *)raw; ++s->calls; assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_value_string(out, "a@b.c") == FVM_OK); return s->result;
}
static FVM_Result existing(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ fvm_value_number(out,77);return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        for (unsigned route = 0; route < 2; ++route) {
            FVM_Value arg, out; assert(fvm_value_string(&arg, "user@somedomain.com") == FVM_OK);
            if (version == 4) { invoke(c, "escape", &arg, 1, route != 0, FVM_UNSUPPORTED); invoke(c, "unescape", &arg, 1, route != 0, FVM_UNSUPPORTED); continue; }
            out = invoke(c, "escape", &arg, 1, route != 0); same(out,"user%40somedomain%2Ecom",sizeof("user%40somedomain%2Ecom")-1);
            out = invoke(c, "unescape", &out, 1, route != 0); same(out,"user@somedomain.com",sizeof("user@somedomain.com")-1);
            assert(invoke(c, "escape", 0, 0, route != 0).type == FVM_VALUE_UNDEFINED);
            assert(invoke(c, "unescape", 0, 0, route != 0).type == FVM_VALUE_UNDEFINED);
            fvm_value_undefined(&arg); out = invoke(c, "escape", &arg, 1, route != 0);
            same(out,version && version < 7 ? "" : "undefined",version && version < 7 ? 0 : 9);
            assert(fvm_value_string(&arg, "abc") == FVM_OK);
            if (!route) {
                c->gc_collecting = true; invoke(c,"escape",&arg,1,false,FVM_INVALID_ARGUMENT); c->gc_collecting = false;
                c->gc_releasing = true; invoke(c,"unescape",&arg,1,false,FVM_INVALID_ARGUMENT); c->gc_releasing = false;
                c->destroying = true; invoke(c,"escape",&arg,1,false,FVM_INVALID_ARGUMENT); c->destroying = false;
            }
            assert(fvm_value_string(&arg, "AZaz09 @*_+-./%") == FVM_OK);
            out = invoke(c, "escape", &arg, 1, route != 0);
            const char expected[] = "AZaz09%20%40%2A%5F%2B%2D%2E%2F%25"; same(out,expected,sizeof(expected)-1);
            assert(fvm_value_string(&arg, "%2b+%2520") == FVM_OK);
            out = invoke(c, "unescape", &arg, 1, route != 0); same(out,"+ %20",5);
            const unsigned short units[] = {0, 'A', 0xe9, 0x4e2d, 0xd83d, 0xde00, 0xd800, 'Z', 0xdfff};
            assert(fvm_value_string_utf16(c, units, sizeof(units)/sizeof(units[0]), &arg) == FVM_OK);
            out = invoke(c, "escape", &arg, 1, route != 0);
            const char wide[] = "%00A%C3%A9%E4%B8%AD%F0%9F%98%80%ED%A0%80Z%ED%BF%BF"; same(out,wide,sizeof(wide)-1);
            out = invoke(c, "unescape", &out, 1, route != 0);
            unsigned short actual[16]; size_t size = 0; assert(fvm_value_get_utf16(&out, actual, 16, &size) == FVM_OK);
            assert(size == sizeof(units)/sizeof(units[0]) && !std::memcmp(units, actual, sizeof(units)));
            const char *bad[] = {"%", "%0", "%GG", "%u0041", "%FF", "%C0%80", "%E2%82", "%F4%90%80%80"};
            for (unsigned i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) { assert(fvm_value_string(&arg,bad[i]) == FVM_OK); invoke(c,"unescape",&arg,1,route != 0,FVM_UNSUPPORTED); }
            unsigned long limit = rt->config.instruction_limit; rt->config.instruction_limit = 2;
            assert(fvm_value_string(&arg, "abc") == FVM_OK); invoke(c,"escape",&arg,1,route != 0,FVM_ERROR);
            assert(fvm_value_string(&arg, "%41") == FVM_OK); invoke(c,"unescape",&arg,1,route != 0,FVM_ERROR);
            rt->config.instruction_limit = limit;
            FVM_Object *o = 0; assert(fvm_object_create(c,&o) == FVM_OK);
            State state; state.calls = 0; state.result = FVM_OK;
            assert(fvm_object_define_method_rooted(c,o,"toString",text,&state,0) == FVM_OK); o->methods["toString"].collector_entry = true;
            fvm_value_undefined(&arg); arg.type = FVM_VALUE_OBJECT; arg.object = o;
            out = invoke(c,"escape",&arg,1,route != 0); same(out,"a%40b%2Ec",9); assert(state.calls == 1);
            state.result = FVM_UNSUPPORTED; invoke(c,"escape",&arg,1,route != 0,FVM_UNSUPPORTED); assert(state.calls == 2);
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->string_depth);
        }
        fvm_context_destroy(c);
    }
    /* Preflight preserves exact and SWF5/6 case-folded user bindings before publication. */
    for (unsigned version=5;version<=8;++version) for(unsigned alias=0;alias<2;++alias) for(unsigned decode=0;decode<2;++decode) for(unsigned slot=0;slot<2;++slot) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        const char *name=decode?(alias?"uNeScApE":"unescape"):(alias?"eScApE":"escape");
        if(!slot) assert(fvm_register_function(c,name,existing,0)==FVM_OK);
        else assert(fvm_object_define_method_rooted(c,c->global,name,existing,0,0)==FVM_OK);
        const bool conflict=!alias || version<=6;
        assert(fvm_standard_install(c)==(conflict?FVM_INVALID_ARGUMENT:FVM_OK));
        assert(c->standard_library!=conflict);
        assert((slot?c->global->methods:c->functions).find(fvm_detail::string(name))->second.callback==existing);
        FVM_Value result;assert(fvm_call(c,name,0,0,&result)==FVM_OK && result.number==77);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
