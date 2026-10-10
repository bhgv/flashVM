#include "../core/runtime.hpp"
#include "flashvm/avm1.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
struct Action { unsigned char op; size_t offset, next, payload, length; };
static unsigned int u16(const unsigned char *p) { return (unsigned int)p[0] | ((unsigned int)p[1]<<8); }
static unsigned long u32(const unsigned char *p) { return (unsigned long)p[0] | ((unsigned long)p[1]<<8) | ((unsigned long)p[2]<<16) | ((unsigned long)p[3]<<24); }
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size, FVM_Value *returned, std::map<std::string, FVM_Value> *locals, FVM_Object *self);
/* A call frame owns parameter bindings and its receiver; script depth is restored
 * even when the interpreter reports an error. */
static FVM_Result call_script(FVM_Context *c, const FVM_ScriptFunction &fn,
                              unsigned int argc, const FVM_Value *args,
                              FVM_Object *receiver, FVM_Value *result) {
    if (c->script_depth>=64) return FVM_ERROR;
    std::map<std::string,FVM_Value> frame;
    try {
        for (size_t i=0;i<fn.parameters.size();++i) {
            FVM_Value value; fvm_value_undefined(&value);
            if (i<argc) value=args[i];
            frame[fn.parameters[i]]=value;
        }
    } catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
    ++c->script_depth;
    FVM_Result status=execute(c,&fn.body[0],fn.body.size(),result,&frame,receiver);
    --c->script_depth;
    return status;
}
static FVM_Result execute(FVM_Context *c, const unsigned char *data, size_t size, FVM_Value *returned, std::map<std::string, FVM_Value> *locals, FVM_Object *self) {
    std::vector<Action> actions;
    std::vector<FVM_Value> stack;
    size_t pos=0, pc=0, i;
    unsigned long steps=0;
    if (!c || (!data && size)) return FVM_INVALID_ARGUMENT;
    if (returned) fvm_value_undefined(returned);
    try {
        while (pos<size) {
            Action a; a.offset=pos; a.op=data[pos++]; a.payload=pos; a.length=0;
            if (a.op>=0x80) {
                if (size-pos<2) return FVM_INVALID_ARGUMENT;
                a.length=u16(data+pos); pos+=2; a.payload=pos;
                if (a.length>size-pos) return FVM_INVALID_ARGUMENT;
                pos+=a.length;
            }
            a.next=pos; actions.push_back(a);
            if (!a.op) break;
        }
        if (actions.empty() || actions.back().op!=0 || pos!=size) return FVM_INVALID_ARGUMENT;
        while (pc<actions.size()) {
            const Action &a=actions[pc];
            if (c->runtime->config.instruction_limit && ++steps>c->runtime->config.instruction_limit) return FVM_ERROR;
            if (a.op==0) return FVM_OK;
            if (a.op==0x3e) { /* Return */
                if (stack.empty()) return FVM_ERROR;
                if (returned) *returned=stack.back();
                return FVM_OK;
            }
            if (a.op==0x9b) { /* DefineFunction: named, zero parameters */
                size_t p=a.payload, end=p+a.length, start=p;
                while (p<end && data[p]) ++p;
                if (p==end || p==start) return FVM_INVALID_ARGUMENT;
                std::string name((const char *)(data+start),p-start);
                ++p;
                if (end-p<4) return FVM_INVALID_ARGUMENT;
                unsigned int count=u16(data+p); p+=2;
                FVM_ScriptFunction fn;
                unsigned int param;
                for (param=0;param<count;++param) {
                    size_t begin=p;
                    while (p<end && data[p]) ++p;
                    if (p==end) return FVM_INVALID_ARGUMENT;
                    fn.parameters.push_back(std::string((const char *)(data+begin),p-begin));
                    ++p;
                }
                if (end-p<2) return FVM_INVALID_ARGUMENT;
                unsigned int body_size=u16(data+p); p+=2;
                if (body_size!=end-p) return FVM_INVALID_ARGUMENT;
                fn.body.assign(data+p,data+end);
                /* A function body can omit End; append one for the decoder. */
                if (fn.body.empty() || fn.body.back()!=0) fn.body.push_back(0);
                c->script_functions[name]=fn;
            } else
 if (a.op==0x96) { /* Push: strings, null, undefined, boolean, double, integer */
                size_t p=a.payload, end=p+a.length;
                while (p<end) {
                    FVM_Value v; fvm_value_undefined(&v);
                    unsigned int type=data[p++];
                    if (type==0) {
                        size_t start=p;
                        while (p<end && data[p]) ++p;
                        if (p==end) return FVM_INVALID_ARGUMENT;
                        v.type=FVM_VALUE_STRING;
                        v.string=(const char *)(data+start); ++p;
                    } else if (type==2) v.type=FVM_VALUE_NULL;
                    else if (type==3) { /* undefined */ }
                    else if (type==5) { if (end-p<1) return FVM_INVALID_ARGUMENT; fvm_value_boolean(&v,data[p++]); }
                    else if (type==7) { if (end-p<4) return FVM_INVALID_ARGUMENT; fvm_value_number(&v,(double)((u32(data+p)&0x80000000UL) ? (long)(u32(data+p)&0x7fffffffUL)-2147483647L-1L : (long)u32(data+p))); p+=4; }
                    else return FVM_UNSUPPORTED;
                    stack.push_back(v);
                }
            } else if (a.op==0x40) { /* NewObject: limited Object() constructor */
                if (stack.size()<2 || stack.back().type!=FVM_VALUE_STRING || stack[stack.size()-2].type!=FVM_VALUE_NUMBER) return FVM_ERROR;
                if (std::strcmp(stack.back().string,"Object")!=0 || stack[stack.size()-2].number!=0) return FVM_UNSUPPORTED;
                FVM_Object *o=0; FVM_Result r=fvm_object_create(c,&o);
                if (r!=FVM_OK) return r;
                stack.pop_back(); stack.pop_back();
                FVM_Value v; fvm_value_undefined(&v); v.type=FVM_VALUE_OBJECT; v.object=o; stack.push_back(v);
            } else if (a.op==0x4e) { /* GetMember */
                if (stack.size()<2 || stack.back().type!=FVM_VALUE_STRING || stack[stack.size()-2].type!=FVM_VALUE_OBJECT) return FVM_ERROR;
                FVM_Value v; FVM_Result r=fvm_object_get(c,stack[stack.size()-2].object,stack.back().string,&v);
                if (r!=FVM_OK) return r;
                stack.pop_back(); stack.back()=v;
            } else if (a.op==0x4f) { /* SetMember */
                if (stack.size()<3 || stack[stack.size()-2].type!=FVM_VALUE_STRING || stack[stack.size()-3].type!=FVM_VALUE_OBJECT) return FVM_ERROR;
                FVM_Result r=fvm_object_set(c,stack[stack.size()-3].object,stack[stack.size()-2].string,&stack.back());
                if (r!=FVM_OK) return r;
                stack.resize(stack.size()-3);
            } else if (a.op==0x4c) { /* PushDuplicate */
                if (stack.empty()) return FVM_ERROR;
                stack.push_back(stack.back());
            } else if (a.op==0x4d) { /* StackSwap */
                if (stack.size()<2) return FVM_ERROR;
                FVM_Value tmp=stack.back();
                stack.back()=stack[stack.size()-2];
                stack[stack.size()-2]=tmp;
            } else if (a.op==0x12) { /* Not */
                if (stack.empty()) return FVM_ERROR;
                FVM_Value &v=stack.back();
                int truth=(v.type==FVM_VALUE_BOOLEAN && v.boolean) ||
                          (v.type==FVM_VALUE_NUMBER && v.number!=0) ||
                          (v.type==FVM_VALUE_STRING && v.string && v.string[0]) ||
                          v.type==FVM_VALUE_OBJECT;
                fvm_value_boolean(&v,!truth);
            } else if (a.op==0x0e || a.op==0x0f || a.op==0x10 || a.op==0x11) {
                /* Equals, Less, And, Or: deliberately limited primitive semantics. */
                if (stack.size()<2) return FVM_ERROR;
                FVM_Value rhs=stack.back(); stack.pop_back();
                FVM_Value &lhs=stack.back();
                if (lhs.type!=FVM_VALUE_NUMBER || rhs.type!=FVM_VALUE_NUMBER)
                    return FVM_UNSUPPORTED;
                int value=0;
                if (a.op==0x0e) value=(lhs.number==rhs.number);
                if (a.op==0x0f) value=(lhs.number<rhs.number);
                if (a.op==0x10) value=(lhs.number!=0 && rhs.number!=0);
                if (a.op==0x11) value=(lhs.number!=0 || rhs.number!=0);
                fvm_value_boolean(&lhs,value);
            } else if (a.op==0x17) { /* Pop */
                if (stack.empty()) return FVM_ERROR;
                stack.pop_back();
            } else if (a.op==0x3c || a.op==0x41) { /* DefineLocal / DeclareLocal */
                if (!locals) return FVM_UNSUPPORTED;
                if (stack.empty() || stack.back().type!=FVM_VALUE_STRING) return FVM_ERROR;
                if (a.op==0x3c && stack.size()<2) return FVM_ERROR;
                std::string name=stack[a.op==0x3c ? stack.size()-2 : stack.size()-1].string;
                FVM_Value v; fvm_value_undefined(&v);
                if (a.op==0x3c) v=stack.back();
                (*locals)[name]=v;
                stack.resize(stack.size()-(a.op==0x3c ? 2 : 1));
            } else if (a.op==0x1c) { /* GetVariable */
                if (stack.empty() || stack.back().type!=FVM_VALUE_STRING) return FVM_ERROR;
                FVM_Value v; FVM_Result r=FVM_OK;
                std::map<std::string,FVM_Value>::iterator local=locals ? locals->find(stack.back().string) : std::map<std::string,FVM_Value>::iterator();
                if (locals && local!=locals->end()) v=local->second;
                else if (self && std::strcmp(stack.back().string,"this")==0) {
                    fvm_value_undefined(&v); v.type=FVM_VALUE_OBJECT; v.object=self;
                } else r=fvm_get_global(c,stack.back().string,&v);
                if (r!=FVM_OK) return r;
                stack.back()=v;
            } else if (a.op==0x1d) { /* SetVariable: name, value */
                if (stack.size()<2 || stack[stack.size()-2].type!=FVM_VALUE_STRING) return FVM_ERROR;
                std::string name=stack[stack.size()-2].string;
                FVM_Result r=FVM_OK;
                if (locals && locals->find(name)!=locals->end()) (*locals)[name]=stack.back();
                else r=fvm_set_global(c,name.c_str(),&stack.back());
                if (r!=FVM_OK) return r;
                stack.pop_back(); stack.pop_back();
            } else if (a.op==0x0a || a.op==0x0b || a.op==0x0c || a.op==0x0d) {
                if (stack.size()<2) return FVM_ERROR;
                FVM_Value b=stack.back(); stack.pop_back(); FVM_Value &v=stack.back();
                if (v.type!=FVM_VALUE_NUMBER || b.type!=FVM_VALUE_NUMBER) return FVM_UNSUPPORTED;
                if (a.op==0x0a) v.number+=b.number;
                if (a.op==0x0b) v.number-=b.number;
                if (a.op==0x0c) v.number*=b.number;
                if (a.op==0x0d) v.number/=b.number;
            } else if (a.op==0x52) { /* CallMethod: args, argc, receiver, method name */
                if (stack.size()<3 || stack.back().type!=FVM_VALUE_STRING ||
                    stack[stack.size()-2].type!=FVM_VALUE_OBJECT ||
                    stack[stack.size()-3].type!=FVM_VALUE_NUMBER) return FVM_ERROR;
                std::string name=stack.back().string;
                FVM_Object *receiver=stack[stack.size()-2].object;
                double n=stack[stack.size()-3].number;
                if (n<0 || n>65535 || n!=(unsigned int)n || stack.size()<(size_t)n+3) return FVM_ERROR;
                unsigned int argc=(unsigned int)n;
                std::vector<FVM_Value> args(argc);
                for (i=0;i<argc;++i) args[i]=stack[stack.size()-4-i];
                FVM_Value result; fvm_value_undefined(&result);
                FVM_Result r;
                std::map<std::string, FVM_ScriptFunction>::const_iterator script=c->script_functions.find(name);
                if (script!=c->script_functions.end())
                    r=call_script(c,script->second,argc,argc?&args[0]:0,receiver,&result);
                else r=fvm_object_call(c,receiver,name.c_str(),argc,argc?&args[0]:0,&result);
                if (r!=FVM_OK) return r;
                stack.resize(stack.size()-argc-3); stack.push_back(result);
            } else if (a.op==0x3d) { /* CallFunction: args, argc, name (top) */
                if (stack.size()<2 || stack.back().type!=FVM_VALUE_STRING || stack[stack.size()-2].type!=FVM_VALUE_NUMBER) return FVM_ERROR;
                std::string name=stack.back().string;
                double n=stack[stack.size()-2].number;
                if (n<0 || n>65535 || n!=(unsigned int)n || stack.size()<(size_t)n+2) return FVM_ERROR;
                unsigned int argc=(unsigned int)n;
                std::vector<FVM_Value> args(argc);
                for (i=0;i<argc;++i) args[i]=stack[stack.size()-3-i];
                FVM_Value result; FVM_Result r;
                std::map<std::string, FVM_ScriptFunction>::const_iterator fn=c->script_functions.find(name);
                if (fn!=c->script_functions.end()) {
                    r=call_script(c,fn->second,argc,argc?&args[0]:0,c->global,&result);
                } else r=fvm_call(c,name.c_str(),argc,argc?&args[0]:0,&result);
                if (r!=FVM_OK) return r;
                stack.resize(stack.size()-argc-2); stack.push_back(result);
            } else if (a.op==0x99 || a.op==0x9d) { /* Jump / If */
                if (a.length!=2) return FVM_INVALID_ARGUMENT;
                int take=1;
                if (a.op==0x9d) {
                    if (stack.empty()) return FVM_ERROR;
                    FVM_Value v=stack.back(); stack.pop_back();
                    take=(v.type==FVM_VALUE_BOOLEAN && v.boolean) || (v.type==FVM_VALUE_NUMBER && v.number!=0);
                }
                if (take) {
                    long delta=(long)u16(data+a.payload);
                    if (delta>=32768) delta-=65536;
                    if (delta<0 && (size_t)(-delta)>a.next) return FVM_INVALID_ARGUMENT;
                    size_t target=delta<0?a.next-(size_t)(-delta):a.next+(size_t)delta;
                    for (i=0;i<actions.size();++i) if (actions[i].offset==target) break;
                    if (i==actions.size()) return FVM_INVALID_ARGUMENT;
                    pc=i; continue;
                }
            } else return FVM_UNSUPPORTED;
            ++pc;
        }
    } catch (const std::bad_alloc &) { return FVM_OUT_OF_MEMORY; }
      catch (...) { return FVM_ERROR; }
    return FVM_INVALID_ARGUMENT;
}
}
extern "C" FVM_Result fvm_avm1_execute(FVM_Context *c, const void *data, size_t size) {
    FVM_AVM1Info info;
    info.struct_size=sizeof(info);
    FVM_Result r=fvm_avm1_inspect(data,size,&info);
    if (r!=FVM_OK) return r;
    return execute(c,(const unsigned char *)data,size,0,0,c->global);
}
extern "C" FVM_Result fvm_swf_execute_actions(FVM_Context *c) {
    if (!c) return FVM_INVALID_ARGUMENT;
    if (!c->swf_loaded) return FVM_ERROR;
    const unsigned char *p=&c->swf_bytes[0];
    size_t size=c->swf_bytes.size();
    size_t rect=(5+4*(p[8]>>3)+7)/8;
    size_t pos=8+rect+4;
    while (pos<size) {
        unsigned int header=u16(p+pos); pos+=2;
        unsigned int code=header>>6; size_t len=header&63;
        if (len==63) { len=(size_t)u32(p+pos); pos+=4; }
        if (code==12) {
            FVM_Result r=fvm_avm1_execute(c,p+pos,len);
            if (r!=FVM_OK) return r;
        }
        pos+=len;
        if (!code) break;
    }
    return FVM_OK;
}
